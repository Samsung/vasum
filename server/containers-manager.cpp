/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Jan Olszak <j.olszak@samsung.com>
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License
 */

/**
 * @file
 * @author  Jan Olszak (j.olszak@samsung.com)
 * @brief   Definition of the class for managing containers
 */

#include "config.hpp"

#include "host-dbus-definitions.hpp"
#include "common-dbus-definitions.hpp"
#include "container-dbus-definitions.hpp"
#include "containers-manager.hpp"
#include "container-admin.hpp"
#include "exception.hpp"

#include "utils/paths.hpp"
#include "logger/logger.hpp"
#include "config/manager.hpp"
#include "dbus/exception.hpp"
#include "utils/fs.hpp"
#include "utils/img.hpp"
#include "utils/environment.hpp"

#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <cassert>
#include <string>
#include <climits>


namespace security_containers {


namespace {

bool regexMatchVector(const std::string& str, const std::vector<boost::regex>& v)
{
    for (const boost::regex& toMatch: v) {
        if (boost::regex_match(str, toMatch)) {
            return true;
        }
    }

    return false;
}

const std::string HOST_ID = "host";
const std::string CONTAINER_TEMPLATE_CONFIG_PATH = "template.conf";

const boost::regex CONTAINER_NAME_REGEX("~NAME~");
const boost::regex CONTAINER_UUID_REGEX("~UUID~");
const boost::regex CONTAINER_IP_THIRD_OCTET_REGEX("~IP~");

const unsigned int CONTAINER_IP_BASE_THIRD_OCTET = 100;

} // namespace

ContainersManager::ContainersManager(const std::string& managerConfigPath): mDetachOnExit(false)
{
    LOGD("Instantiating ContainersManager object...");

    mConfigPath = managerConfigPath;
    config::loadFromFile(mConfigPath, mConfig);

    mProxyCallPolicy.reset(new ProxyCallPolicy(mConfig.proxyCallRules));

    using namespace std::placeholders;
    mHostConnection.setProxyCallCallback(bind(&ContainersManager::handleProxyCall,
                                              this, HOST_ID, _1, _2, _3, _4, _5, _6, _7));

    mHostConnection.setGetContainerDbusesCallback(bind(
                &ContainersManager::handleGetContainerDbuses, this, _1));

    mHostConnection.setGetContainerIdsCallback(bind(&ContainersManager::handleGetContainerIdsCall,
                                                    this, _1));

    mHostConnection.setGetActiveContainerIdCallback(bind(&ContainersManager::handleGetActiveContainerIdCall,
                                                         this, _1));

    mHostConnection.setGetContainerInfoCallback(bind(&ContainersManager::handleGetContainerInfoCall,
                                                     this, _1, _2));

    mHostConnection.setSetActiveContainerCallback(bind(&ContainersManager::handleSetActiveContainerCall,
                                                       this, _1, _2));

    mHostConnection.setAddContainerCallback(bind(&ContainersManager::handleAddContainerCall,
                                                           this, _1, _2));

    for (auto& containerConfig : mConfig.containerConfigs) {
        addContainer(containerConfig);
    }

    // check if default container exists, throw ContainerOperationException if not found
    if (!mConfig.defaultId.empty() && mContainers.find(mConfig.defaultId) == mContainers.end()) {
        LOGE("Provided default container ID " << mConfig.defaultId << " is invalid.");
        throw ContainerOperationException("Provided default container ID " + mConfig.defaultId +
                                          " is invalid.");
    }

    LOGD("ContainersManager object instantiated");

    if (mConfig.inputConfig.enabled) {
        LOGI("Registering input monitor [" << mConfig.inputConfig.device.c_str() << "]");
        mSwitchingSequenceMonitor.reset(
                new InputMonitor(mConfig.inputConfig,
                                 std::bind(&ContainersManager::switchingSequenceMonitorNotify,
                                           this)));
    }


}

ContainersManager::~ContainersManager()
{
    LOGD("Destroying ContainersManager object...");

    if (!mDetachOnExit) {
        try {
            stopAll();
        } catch (ServerException&) {
            LOGE("Failed to stop all of the containers");
        }
    }

    LOGD("ContainersManager object destroyed");
}

void ContainersManager::addContainer(const std::string& containerConfig)
{
    std::string baseConfigPath = utils::dirName(mConfigPath);
    std::string containerConfigPath = utils::getAbsolutePath(containerConfig, baseConfigPath);

    LOGT("Creating Container " << containerConfigPath);
    std::unique_ptr<Container> c(new Container(mConfig.containersPath,
                                               containerConfigPath,
                                               mConfig.lxcTemplatePrefix,
                                               mConfig.runMountPointPrefix));
    const std::string id = c->getId();
    if (id == HOST_ID) {
        throw ContainerOperationException("Cannot use reserved container ID");
    }

    using namespace std::placeholders;
    c->setNotifyActiveContainerCallback(bind(&ContainersManager::notifyActiveContainerHandler,
                                             this, id, _1, _2));

    c->setDisplayOffCallback(bind(&ContainersManager::displayOffHandler,
                                  this, id));

    c->setFileMoveRequestCallback(bind(&ContainersManager::handleContainerMoveFileRequest,
                                            this, id, _1, _2, _3));

    c->setProxyCallCallback(bind(&ContainersManager::handleProxyCall,
                                 this, id, _1, _2, _3, _4, _5, _6, _7));

    c->setDbusStateChangedCallback(bind(&ContainersManager::handleDbusStateChanged,
                                        this, id, _1));

    mContainers.insert(ContainerMap::value_type(id, std::move(c)));
}

void ContainersManager::focus(const std::string& containerId)
{
    /* try to access the object first to throw immediately if it doesn't exist */
    ContainerMap::mapped_type& foregroundContainer = mContainers.at(containerId);

    if (!foregroundContainer->activateVT()) {
        LOGE("Failed to activate containers VT. Aborting focus.");
        return;
    }

    for (auto& container : mContainers) {
        LOGD(container.second->getId() << ": being sent to background");
        container.second->goBackground();
    }
    mConfig.foregroundId = foregroundContainer->getId();
    LOGD(mConfig.foregroundId << ": being sent to foreground");
    foregroundContainer->goForeground();
}

void ContainersManager::startAll()
{
    LOGI("Starting all containers");

    bool isForegroundFound = false;

    for (auto& container : mContainers) {
        container.second->start();

        if (container.first == mConfig.foregroundId) {
            isForegroundFound = true;
            LOGI(container.second->getId() << ": set as the foreground container");
            container.second->goForeground();
        }
    }

    if (!isForegroundFound) {
        auto foregroundIterator = std::min_element(mContainers.begin(), mContainers.end(),
                                                   [](ContainerMap::value_type &c1, ContainerMap::value_type &c2) {
                                                       return c1.second->getPrivilege() < c2.second->getPrivilege();
                                                   });

        if (foregroundIterator != mContainers.end()) {
            mConfig.foregroundId = foregroundIterator->second->getId();
            LOGI(mConfig.foregroundId << ": no foreground container configured, setting one with highest priority");
            foregroundIterator->second->goForeground();
        }
    }
}

void ContainersManager::stopAll()
{
    LOGI("Stopping all containers");

    for (auto& container : mContainers) {
        container.second->stop();
    }
}

std::string ContainersManager::getRunningForegroundContainerId()
{
    for (auto& container : mContainers) {
        if (container.first == mConfig.foregroundId &&
            container.second->isRunning()) {
            return container.first;
        }
    }
    return std::string();
}

std::string ContainersManager::getNextToForegroundContainerId()
{
    // handles case where there is no next container
    if (mContainers.size() < 2) {
        return std::string();
    }

    for (auto it = mContainers.begin(); it != mContainers.end(); ++it) {
        if (it->first == mConfig.foregroundId &&
            it->second->isRunning()) {
            auto nextIt = std::next(it);
            if (nextIt != mContainers.end()) {
                return nextIt->first;
            }
        }
    }
    return mContainers.begin()->first;
}

void ContainersManager::switchingSequenceMonitorNotify()
{
    LOGI("switchingSequenceMonitorNotify() called");

    auto nextContainerId = getNextToForegroundContainerId();

    if (!nextContainerId.empty()) {
        focus(nextContainerId);
    }
}


void ContainersManager::setContainersDetachOnExit()
{
    mDetachOnExit = true;

    for (auto& container : mContainers) {
        container.second->setDetachOnExit();
    }
}

void ContainersManager::notifyActiveContainerHandler(const std::string& caller,
                                                     const std::string& application,
                                                     const std::string& message)
{
    LOGI("notifyActiveContainerHandler(" << caller << ", " << application << ", " << message
         << ") called");
    try {
        const std::string activeContainer = getRunningForegroundContainerId();
        if (!activeContainer.empty() && caller != activeContainer) {
            mContainers[activeContainer]->sendNotification(caller, application, message);
        }
    } catch(const SecurityContainersException&) {
        LOGE("Notification from " << caller << " hasn't been sent");
    }
}

void ContainersManager::displayOffHandler(const std::string& /*caller*/)
{
    // get config of currently set container and switch if switchToDefaultAfterTimeout is true
    const std::string activeContainerName = getRunningForegroundContainerId();
    const auto& activeContainer = mContainers.find(activeContainerName);

    if (activeContainer != mContainers.end() &&
        activeContainer->second->isSwitchToDefaultAfterTimeoutAllowed()) {
        LOGI("Switching to default container " << mConfig.defaultId);
        focus(mConfig.defaultId);
    }
}

void ContainersManager::handleContainerMoveFileRequest(const std::string& srcContainerId,
                                                       const std::string& dstContainerId,
                                                       const std::string& path,
                                                       dbus::MethodResultBuilder::Pointer result)
{
    // TODO: this implementation is only a placeholder.
    // There are too many unanswered questions and security concerns:
    // 1. What about mount namespace, host might not see the source/destination
    //    file. The file might be a different file from a host perspective.
    // 2. Copy vs move (speed and security concerns over already opened FDs)
    // 3. Access to source and destination files - DAC, uid/gig
    // 4. Access to source and destintation files - MAC, smack
    // 5. Destination file uid/gid assignment
    // 6. Destination file smack label assignment
    // 7. Verifiability of the source path

    // NOTE: other possible implementations include:
    // 1. Sending file descriptors opened directly in each container through DBUS
    //    using something like g_dbus_message_set_unix_fd_list()
    // 2. SCS forking and calling setns(MNT) in each container and opening files
    //    by itself, then passing FDs to the main process
    // Now when the main process has obtained FDs (by either of those methods)
    // it can do the copying by itself.

    LOGI("File move requested\n"
         << "src: " << srcContainerId << "\n"
         << "dst: " << dstContainerId << "\n"
         << "path: " << path);

    ContainerMap::const_iterator srcIter = mContainers.find(srcContainerId);
    if (srcIter == mContainers.end()) {
        LOGE("Source container '" << srcContainerId << "' not found");
        return;
    }
    Container& srcContainer = *srcIter->second;

    ContainerMap::const_iterator dstIter = mContainers.find(dstContainerId);
    if (dstIter == mContainers.end()) {
        LOGE("Destination container '" << dstContainerId << "' not found");
        result->set(g_variant_new("(s)", api::container::FILE_MOVE_DESTINATION_NOT_FOUND.c_str()));
        return;
    }
    Container& dstContanier = *dstIter->second;

    if (srcContainerId == dstContainerId) {
        LOGE("Cannot send a file to yourself");
        result->set(g_variant_new("(s)", api::container::FILE_MOVE_WRONG_DESTINATION.c_str()));
        return;
    }

    if (!regexMatchVector(path, srcContainer.getPermittedToSend())) {
        LOGE("Source container has no permissions to send the file: " << path);
        result->set(g_variant_new("(s)", api::container::FILE_MOVE_NO_PERMISSIONS_SEND.c_str()));
        return;
    }

    if (!regexMatchVector(path, dstContanier.getPermittedToRecv())) {
        LOGE("Destination container has no permissions to receive the file: " << path);
        result->set(g_variant_new("(s)", api::container::FILE_MOVE_NO_PERMISSIONS_RECEIVE.c_str()));
        return;
    }

    namespace fs = boost::filesystem;
    std::string srcPath = fs::absolute(srcContainerId, mConfig.containersPath).string() + path;
    std::string dstPath = fs::absolute(dstContainerId, mConfig.containersPath).string() + path;

    if (!utils::moveFile(srcPath, dstPath)) {
        LOGE("Failed to move the file: " << path);
        result->set(g_variant_new("(s)", api::container::FILE_MOVE_FAILED.c_str()));
    } else {
        result->set(g_variant_new("(s)", api::container::FILE_MOVE_SUCCEEDED.c_str()));
        try {
            dstContanier.sendNotification(srcContainerId, path, api::container::FILE_MOVE_SUCCEEDED);
        } catch (ServerException&) {
            LOGE("Notification to '" << dstContainerId << "' has not been sent");
        }
    }
}

void ContainersManager::handleProxyCall(const std::string& caller,
                                        const std::string& target,
                                        const std::string& targetBusName,
                                        const std::string& targetObjectPath,
                                        const std::string& targetInterface,
                                        const std::string& targetMethod,
                                        GVariant* parameters,
                                        dbus::MethodResultBuilder::Pointer result)
{
    if (!mProxyCallPolicy->isProxyCallAllowed(caller,
                                              target,
                                              targetBusName,
                                              targetObjectPath,
                                              targetInterface,
                                              targetMethod)) {
        LOGW("Forbidden proxy call; " << caller << " -> " << target << "; " << targetBusName
                << "; " << targetObjectPath << "; " << targetInterface << "; " << targetMethod);
        result->setError(api::ERROR_FORBIDDEN, "Proxy call forbidden");
        return;
    }

    LOGI("Proxy call; " << caller << " -> " << target << "; " << targetBusName
            << "; " << targetObjectPath << "; " << targetInterface << "; " << targetMethod);

    auto asyncResultCallback = [result](dbus::AsyncMethodCallResult& asyncMethodCallResult) {
        try {
            GVariant* targetResult = asyncMethodCallResult.get();
            result->set(g_variant_new("(v)", targetResult));
        } catch (dbus::DbusException& e) {
            result->setError(api::ERROR_FORWARDED, e.what());
        }
    };

    if (target == HOST_ID) {
        mHostConnection.proxyCallAsync(targetBusName,
                                       targetObjectPath,
                                       targetInterface,
                                       targetMethod,
                                       parameters,
                                       asyncResultCallback);
        return;
    }

    ContainerMap::const_iterator targetIter = mContainers.find(target);
    if (targetIter == mContainers.end()) {
        LOGE("Target container '" << target << "' not found");
        result->setError(api::ERROR_UNKNOWN_ID, "Unknown proxy call target");
        return;
    }

    Container& targetContainer = *targetIter->second;
    targetContainer.proxyCallAsync(targetBusName,
                                   targetObjectPath,
                                   targetInterface,
                                   targetMethod,
                                   parameters,
                                   asyncResultCallback);
}

void ContainersManager::handleGetContainerDbuses(dbus::MethodResultBuilder::Pointer result)
{
    std::vector<GVariant*> entries;
    for (auto& container : mContainers) {
        GVariant* containerId = g_variant_new_string(container.first.c_str());
        GVariant* dbusAddress = g_variant_new_string(container.second->getDbusAddress().c_str());
        GVariant* entry = g_variant_new_dict_entry(containerId, dbusAddress);
        entries.push_back(entry);
    }
    GVariant* dict = g_variant_new_array(G_VARIANT_TYPE("{ss}"), entries.data(), entries.size());
    result->set(g_variant_new("(*)", dict));
}

void ContainersManager::handleDbusStateChanged(const std::string& containerId,
                                               const std::string& dbusAddress)
{
    mHostConnection.signalContainerDbusState(containerId, dbusAddress);
}

void ContainersManager::handleGetContainerIdsCall(dbus::MethodResultBuilder::Pointer result)
{
    std::vector<GVariant*> containerIds;
    for(auto& container: mContainers){
        containerIds.push_back(g_variant_new_string(container.first.c_str()));
    }

    GVariant* array = g_variant_new_array(G_VARIANT_TYPE("s"),
                                          containerIds.data(),
                                          containerIds.size());
    result->set(g_variant_new("(*)", array));
}

void ContainersManager::handleGetActiveContainerIdCall(dbus::MethodResultBuilder::Pointer result)
{
    LOGI("GetActiveContainerId call");
    if (!mConfig.foregroundId.empty() && mContainers[mConfig.foregroundId]->isRunning()){
        result->set(g_variant_new("(s)", mConfig.foregroundId.c_str()));
    } else {
        result->set(g_variant_new("(s)", ""));
    }
}

void ContainersManager::handleGetContainerInfoCall(const std::string& id,
                                                   dbus::MethodResultBuilder::Pointer result)
{
    LOGI("GetContainerInfo call");
    if (mContainers.count(id) == 0) {
        LOGE("No container with id=" << id);
        result->setError(api::ERROR_UNKNOWN_ID, "No such container id");
        return;
    }
    const auto& container = mContainers[id];
    const char* state;
    //TODO: Use the lookup map.
    if (container->isRunning()) {
        state = "RUNNING";
    } else if (container->isStopped()) {
        state = "STOPPED";
    } else if (container->isPaused()) {
        state = "FROZEN";
    } else {
        LOGE("Unrecognized state of container id=" << id);
        result->setError(api::ERROR_INTERNAL, "Unrecognized state of container");
        return;
    }
    const std::string rootPath = boost::filesystem::absolute(id, mConfig.containersPath).string();
    result->set(g_variant_new("((siss))",
                              id.c_str(),
                              container->getVT(),
                              state,
                              rootPath.c_str()));
}

void ContainersManager::handleSetActiveContainerCall(const std::string& id,
                                                     dbus::MethodResultBuilder::Pointer result)
{
    LOGI("SetActiveContainer call; Id=" << id );
    auto container = mContainers.find(id);
    if (container == mContainers.end()){
        LOGE("No container with id=" << id );
        result->setError(api::ERROR_UNKNOWN_ID, "No such container id");
        return;
    }

    if (container->second->isStopped()){
        LOGE("Could not activate a stopped container");
        result->setError(api::host::ERROR_CONTAINER_STOPPED,
                         "Could not activate a stopped container");
        return;
    }

    focus(id);
    result->setVoid();
}


void ContainersManager::generateNewConfig(const std::string& id,
                                          const std::string& templatePath,
                                          const std::string& resultPath)
{
    namespace fs = boost::filesystem;

    std::string resultFileDir = utils::dirName(resultPath);
    if (!fs::exists(resultFileDir)) {
        if (!utils::createEmptyDir(resultFileDir)) {
            LOGE("Unable to create directory for new config.");
            throw ContainerOperationException("Unable to create directory for new config.");
        }
    }

    fs::path resultFile(resultPath);
    if (fs::exists(resultFile)) {
        LOGT(resultPath << " already exists, removing");
        fs::remove(resultFile);
    }

    std::string config;
    if (!utils::readFileContent(templatePath, config)) {
        LOGE("Failed to read template config file.");
        throw ContainerOperationException("Failed to read template config file.");
    }

    std::string resultConfig = boost::regex_replace(config, CONTAINER_NAME_REGEX, id);

    boost::uuids::uuid u = boost::uuids::random_generator()();
    std::string uuidStr = to_string(u);
    LOGD("uuid: " << uuidStr);
    resultConfig = boost::regex_replace(resultConfig, CONTAINER_UUID_REGEX, uuidStr);

    // generate third IP octet for network config
    std::string thirdOctetStr = std::to_string(CONTAINER_IP_BASE_THIRD_OCTET + mContainers.size() + 1);
    LOGD("ip_third_octet: " << thirdOctetStr);
    resultConfig = boost::regex_replace(resultConfig, CONTAINER_IP_THIRD_OCTET_REGEX, thirdOctetStr);

    if (!utils::saveFileContent(resultPath, resultConfig)) {
        LOGE("Faield to save new config file.");
        throw ContainerOperationException("Failed to save new config file.");
    }

    // restrict new config file so that only owner (security-containers) can write it
    fs::permissions(resultPath, fs::perms::owner_all |
                                fs::perms::group_read |
                                fs::perms::others_read);
}

void ContainersManager::handleAddContainerCall(const std::string& id,
                                               dbus::MethodResultBuilder::Pointer result)
{
    if (id.empty()) {
        LOGE("Failed to add container - invalid name.");
        result->setError(api::host::ERROR_CONTAINER_CREATE_FAILED,
                         "Failed to add container - invalid name.");
        return;
    }

    LOGI("Adding container " << id);

    // TODO: This solution is temporary. It utilizes direct access to config files when creating new
    // containers. Update this handler when config database will appear.
    namespace fs = boost::filesystem;

    boost::system::error_code ec;
    const std::string containerPathStr = utils::createFilePath(mConfig.containersPath, "/", id, "/");

    // check if container does not exist
    if (mContainers.find(id) != mContainers.end()) {
        LOGE("Cannot create " << id << " container - already exists!");
        result->setError(api::host::ERROR_CONTAINER_CREATE_FAILED,
                         "Cannot create " + id + " container - already exists!");
        return;
    }

    // copy container image if config contains path to image
    LOGT("image path: " << mConfig.containerImagePath);
    if (!mConfig.containerImagePath.empty()) {
        auto copyImageContentsWrapper = std::bind(&utils::copyImageContents,
                                                  mConfig.containerImagePath,
                                                  containerPathStr);

        if (!utils::launchAsRoot(copyImageContentsWrapper)) {
            LOGE("Failed to copy container image.");
            result->setError(api::host::ERROR_CONTAINER_CREATE_FAILED,
                            "Failed to copy container image.");
            return;
        }
    }

    // generate paths to new configuration files
    std::string baseDir = utils::dirName(mConfigPath);
    std::string configDir = utils::getAbsolutePath(mConfig.containerNewConfigPrefix, baseDir);
    std::string templateDir = utils::getAbsolutePath(mConfig.containerTemplatePath, baseDir);

    std::string configPath = utils::createFilePath(templateDir, "/", CONTAINER_TEMPLATE_CONFIG_PATH);
    std::string newConfigPath = utils::createFilePath(configDir, "/containers/", id + ".conf");

    auto removeAllWrapper = [](const std::string& path) {
        try {
            LOGD("Removing copied data");
            fs::remove_all(fs::path(path));
        } catch(const std::exception& e) {
            LOGW("Failed to remove data: " << boost::diagnostic_information(e));
        }
    };

    try {
        LOGI("Generating config from " << configPath << " to " << newConfigPath);
        generateNewConfig(id, configPath, newConfigPath);

    } catch (SecurityContainersException& e) {
        LOGE(e.what());
        utils::launchAsRoot(std::bind(removeAllWrapper, containerPathStr));
        result->setError(api::host::ERROR_CONTAINER_CREATE_FAILED, e.what());
        return;
    }

    LOGT("Adding new container");
    try {
        addContainer(newConfigPath);
    } catch (SecurityContainersException& e) {
        LOGE(e.what());
        utils::launchAsRoot(std::bind(removeAllWrapper, containerPathStr));
        result->setError(api::host::ERROR_CONTAINER_CREATE_FAILED, e.what());
        return;
    }

    auto resultCallback = [this, id, result, containerPathStr, removeAllWrapper](bool succeeded) {
        if (succeeded) {
            focus(id);
            result->setVoid();
        } else {
            LOGE("Failed to start container.");
            utils::launchAsRoot(std::bind(removeAllWrapper, containerPathStr));
            result->setError(api::host::ERROR_CONTAINER_CREATE_FAILED,
                             "Failed to start container.");
        }
    };
    mContainers[id]->startAsync(resultCallback);
}

} // namespace security_containers
