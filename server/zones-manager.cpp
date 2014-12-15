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
 * @brief   Definition of the class for managing zones
 */

#include "config.hpp"

#include "host-dbus-definitions.hpp"
#include "common-dbus-definitions.hpp"
#include "zone-dbus-definitions.hpp"
#include "zones-manager.hpp"
#include "zone-admin.hpp"
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
#include <boost/exception/diagnostic_information.hpp>
#include <cassert>
#include <string>
#include <climits>


namespace vasum {


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
const std::string ZONE_TEMPLATE_CONFIG_PATH = "template.conf";
const std::string ENABLED_FILE_NAME = "enabled";

const boost::regex ZONE_NAME_REGEX("~NAME~");
const boost::regex ZONE_IP_THIRD_OCTET_REGEX("~IP~");

const unsigned int ZONE_IP_BASE_THIRD_OCTET = 100;

} // namespace

ZonesManager::ZonesManager(const std::string& managerConfigPath): mDetachOnExit(false)
{
    LOGD("Instantiating ZonesManager object...");

    mConfigPath = managerConfigPath;
    config::loadFromFile(mConfigPath, mConfig);

    mProxyCallPolicy.reset(new ProxyCallPolicy(mConfig.proxyCallRules));

    using namespace std::placeholders;
    mHostConnection.setProxyCallCallback(bind(&ZonesManager::handleProxyCall,
                                              this, HOST_ID, _1, _2, _3, _4, _5, _6, _7));

    mHostConnection.setGetZoneDbusesCallback(bind(
                &ZonesManager::handleGetZoneDbuses, this, _1));

    mHostConnection.setGetZoneIdsCallback(bind(&ZonesManager::handleGetZoneIdsCall,
                                                    this, _1));

    mHostConnection.setGetActiveZoneIdCallback(bind(&ZonesManager::handleGetActiveZoneIdCall,
                                                         this, _1));

    mHostConnection.setGetZoneInfoCallback(bind(&ZonesManager::handleGetZoneInfoCall,
                                                     this, _1, _2));

    mHostConnection.setDeclareFileCallback(bind(&ZonesManager::handleDeclareFileCall,
                                                this, _1, _2, _3, _4, _5, _6));

    mHostConnection.setDeclareMountCallback(bind(&ZonesManager::handleDeclareMountCall,
                                                 this, _1, _2, _3, _4, _5, _6, _7));

    mHostConnection.setDeclareLinkCallback(bind(&ZonesManager::handleDeclareLinkCall,
                                                this, _1, _2, _3, _4));

    mHostConnection.setSetActiveZoneCallback(bind(&ZonesManager::handleSetActiveZoneCall,
                                                       this, _1, _2));

    mHostConnection.setCreateZoneCallback(bind(&ZonesManager::handleCreateZoneCall,
                                                    this, _1, _2));

    mHostConnection.setDestroyZoneCallback(bind(&ZonesManager::handleDestroyZoneCall,
                                                     this, _1, _2));

    mHostConnection.setLockZoneCallback(bind(&ZonesManager::handleLockZoneCall,
                                                  this, _1, _2));

    mHostConnection.setUnlockZoneCallback(bind(&ZonesManager::handleUnlockZoneCall,
                                                    this, _1, _2));

    for (auto& zoneConfig : mConfig.zoneConfigs) {
        createZone(zoneConfig);
    }

    // check if default zone exists, throw ZoneOperationException if not found
    if (!mConfig.defaultId.empty() && mZones.find(mConfig.defaultId) == mZones.end()) {
        LOGE("Provided default zone ID " << mConfig.defaultId << " is invalid.");
        throw ZoneOperationException("Provided default zone ID " + mConfig.defaultId +
                                          " is invalid.");
    }

    LOGD("ZonesManager object instantiated");

    if (mConfig.inputConfig.enabled) {
        LOGI("Registering input monitor [" << mConfig.inputConfig.device.c_str() << "]");
        mSwitchingSequenceMonitor.reset(
                new InputMonitor(mConfig.inputConfig,
                                 std::bind(&ZonesManager::switchingSequenceMonitorNotify,
                                           this)));
    }


}

ZonesManager::~ZonesManager()
{
    LOGD("Destroying ZonesManager object...");

    if (!mDetachOnExit) {
        try {
            stopAll();
        } catch (ServerException&) {
            LOGE("Failed to stop all of the zones");
        }
    }

    LOGD("ZonesManager object destroyed");
}

void ZonesManager::createZone(const std::string& zoneConfig)
{
    std::string baseConfigPath = utils::dirName(mConfigPath);
    std::string zoneConfigPath = utils::getAbsolutePath(zoneConfig, baseConfigPath);

    LOGT("Creating Zone " << zoneConfigPath);
    std::unique_ptr<Zone> c(new Zone(mConfig.zonesPath,
                                               zoneConfigPath,
                                               mConfig.lxcTemplatePrefix,
                                               mConfig.runMountPointPrefix));
    const std::string id = c->getId();
    if (id == HOST_ID) {
        throw ZoneOperationException("Cannot use reserved zone ID");
    }

    using namespace std::placeholders;
    c->setNotifyActiveZoneCallback(bind(&ZonesManager::notifyActiveZoneHandler,
                                             this, id, _1, _2));

    c->setDisplayOffCallback(bind(&ZonesManager::displayOffHandler,
                                  this, id));

    c->setFileMoveRequestCallback(bind(&ZonesManager::handleZoneMoveFileRequest,
                                            this, id, _1, _2, _3));

    c->setProxyCallCallback(bind(&ZonesManager::handleProxyCall,
                                 this, id, _1, _2, _3, _4, _5, _6, _7));

    c->setDbusStateChangedCallback(bind(&ZonesManager::handleDbusStateChanged,
                                        this, id, _1));

    mZones.insert(ZoneMap::value_type(id, std::move(c)));

    // after zone is created successfully, put a file informing that zones are enabled
    if (mZones.size() == 1) {
        if (!utils::saveFileContent(
                utils::createFilePath(mConfig.zonesPath, "/", ENABLED_FILE_NAME), "")) {
            throw ZoneOperationException(ENABLED_FILE_NAME + ": cannot create.");
        }
    }
}

void ZonesManager::destroyZone(const std::string& zoneId)
{
    // TODO mutex for mZones access
    auto it = mZones.find(zoneId);
    if (it == mZones.end()) {
        LOGE("Failed to destroy zone " << zoneId << ": no such zone");
        throw ZoneOperationException("No such zone");
    }

    // TODO give back the focus
    it->second->setDestroyOnExit();
    mZones.erase(it);

    if (mZones.size() == 0) {
        if (!utils::removeFile(utils::createFilePath(mConfig.zonesPath, "/", ENABLED_FILE_NAME))) {
            LOGE("Failed to remove enabled file.");
        }
    }
}

void ZonesManager::focus(const std::string& zoneId)
{
    /* try to access the object first to throw immediately if it doesn't exist */
    ZoneMap::mapped_type& foregroundZone = mZones.at(zoneId);

    if (!foregroundZone->activateVT()) {
        LOGE("Failed to activate zones VT. Aborting focus.");
        return;
    }

    for (auto& zone : mZones) {
        LOGD(zone.second->getId() << ": being sent to background");
        zone.second->goBackground();
    }
    mConfig.foregroundId = foregroundZone->getId();
    LOGD(mConfig.foregroundId << ": being sent to foreground");
    foregroundZone->goForeground();
}

void ZonesManager::startAll()
{
    LOGI("Starting all zones");

    bool isForegroundFound = false;

    for (auto& zone : mZones) {
        zone.second->start();

        if (zone.first == mConfig.foregroundId) {
            isForegroundFound = true;
            LOGI(zone.second->getId() << ": set as the foreground zone");
            zone.second->goForeground();
        }
    }

    if (!isForegroundFound) {
        auto foregroundIterator = std::min_element(mZones.begin(), mZones.end(),
                                                   [](ZoneMap::value_type &c1, ZoneMap::value_type &c2) {
                                                       return c1.second->getPrivilege() < c2.second->getPrivilege();
                                                   });

        if (foregroundIterator != mZones.end()) {
            mConfig.foregroundId = foregroundIterator->second->getId();
            LOGI(mConfig.foregroundId << ": no foreground zone configured, setting one with highest priority");
            foregroundIterator->second->goForeground();
        }
    }
}

void ZonesManager::stopAll()
{
    LOGI("Stopping all zones");

    for (auto& zone : mZones) {
        zone.second->stop();
    }
}

bool ZonesManager::isPaused(const std::string& zoneId)
{
    auto iter = mZones.find(zoneId);
    if (iter == mZones.end()) {
        LOGE("No such zone id: " << zoneId);
        throw ZoneOperationException("No such zone");
    }

    return iter->second->isPaused();
}

bool ZonesManager::isRunning(const std::string& zoneId)
{
    auto iter = mZones.find(zoneId);
    if (iter == mZones.end()) {
        LOGE("No such zone id: " << zoneId);
        throw ZoneOperationException("No such zone");
    }
    return iter->second->isRunning();
}

std::string ZonesManager::getRunningForegroundZoneId()
{
    for (auto& zone : mZones) {
        if (zone.first == mConfig.foregroundId &&
            zone.second->isRunning()) {
            return zone.first;
        }
    }
    return std::string();
}

std::string ZonesManager::getNextToForegroundZoneId()
{
    // handles case where there is no next zone
    if (mZones.size() < 2) {
        return std::string();
    }

    for (auto it = mZones.begin(); it != mZones.end(); ++it) {
        if (it->first == mConfig.foregroundId &&
            it->second->isRunning()) {
            auto nextIt = std::next(it);
            if (nextIt != mZones.end()) {
                return nextIt->first;
            }
        }
    }
    return mZones.begin()->first;
}

void ZonesManager::switchingSequenceMonitorNotify()
{
    LOGI("switchingSequenceMonitorNotify() called");

    auto nextZoneId = getNextToForegroundZoneId();

    if (!nextZoneId.empty()) {
        focus(nextZoneId);
    }
}


void ZonesManager::setZonesDetachOnExit()
{
    mDetachOnExit = true;

    for (auto& zone : mZones) {
        zone.second->setDetachOnExit();
    }
}

void ZonesManager::notifyActiveZoneHandler(const std::string& caller,
                                                     const std::string& application,
                                                     const std::string& message)
{
    LOGI("notifyActiveZoneHandler(" << caller << ", " << application << ", " << message
         << ") called");
    try {
        const std::string activeZone = getRunningForegroundZoneId();
        if (!activeZone.empty() && caller != activeZone) {
            mZones[activeZone]->sendNotification(caller, application, message);
        }
    } catch(const VasumException&) {
        LOGE("Notification from " << caller << " hasn't been sent");
    }
}

void ZonesManager::displayOffHandler(const std::string& /*caller*/)
{
    // get config of currently set zone and switch if switchToDefaultAfterTimeout is true
    const std::string activeZoneName = getRunningForegroundZoneId();
    const auto& activeZone = mZones.find(activeZoneName);

    if (activeZone != mZones.end() &&
        activeZone->second->isSwitchToDefaultAfterTimeoutAllowed()) {
        LOGI("Switching to default zone " << mConfig.defaultId);
        focus(mConfig.defaultId);
    }
}

void ZonesManager::handleZoneMoveFileRequest(const std::string& srcZoneId,
                                                       const std::string& dstZoneId,
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
    // 1. Sending file descriptors opened directly in each zone through DBUS
    //    using something like g_dbus_message_set_unix_fd_list()
    // 2. VSM forking and calling setns(MNT) in each zone and opening files
    //    by itself, then passing FDs to the main process
    // Now when the main process has obtained FDs (by either of those methods)
    // it can do the copying by itself.

    LOGI("File move requested\n"
         << "src: " << srcZoneId << "\n"
         << "dst: " << dstZoneId << "\n"
         << "path: " << path);

    ZoneMap::const_iterator srcIter = mZones.find(srcZoneId);
    if (srcIter == mZones.end()) {
        LOGE("Source zone '" << srcZoneId << "' not found");
        return;
    }
    Zone& srcZone = *srcIter->second;

    ZoneMap::const_iterator dstIter = mZones.find(dstZoneId);
    if (dstIter == mZones.end()) {
        LOGE("Destination zone '" << dstZoneId << "' not found");
        result->set(g_variant_new("(s)", api::zone::FILE_MOVE_DESTINATION_NOT_FOUND.c_str()));
        return;
    }
    Zone& dstContanier = *dstIter->second;

    if (srcZoneId == dstZoneId) {
        LOGE("Cannot send a file to yourself");
        result->set(g_variant_new("(s)", api::zone::FILE_MOVE_WRONG_DESTINATION.c_str()));
        return;
    }

    if (!regexMatchVector(path, srcZone.getPermittedToSend())) {
        LOGE("Source zone has no permissions to send the file: " << path);
        result->set(g_variant_new("(s)", api::zone::FILE_MOVE_NO_PERMISSIONS_SEND.c_str()));
        return;
    }

    if (!regexMatchVector(path, dstContanier.getPermittedToRecv())) {
        LOGE("Destination zone has no permissions to receive the file: " << path);
        result->set(g_variant_new("(s)", api::zone::FILE_MOVE_NO_PERMISSIONS_RECEIVE.c_str()));
        return;
    }

    namespace fs = boost::filesystem;
    std::string srcPath = fs::absolute(srcZoneId, mConfig.zonesPath).string() + path;
    std::string dstPath = fs::absolute(dstZoneId, mConfig.zonesPath).string() + path;

    if (!utils::moveFile(srcPath, dstPath)) {
        LOGE("Failed to move the file: " << path);
        result->set(g_variant_new("(s)", api::zone::FILE_MOVE_FAILED.c_str()));
    } else {
        result->set(g_variant_new("(s)", api::zone::FILE_MOVE_SUCCEEDED.c_str()));
        try {
            dstContanier.sendNotification(srcZoneId, path, api::zone::FILE_MOVE_SUCCEEDED);
        } catch (ServerException&) {
            LOGE("Notification to '" << dstZoneId << "' has not been sent");
        }
    }
}

void ZonesManager::handleProxyCall(const std::string& caller,
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

    ZoneMap::const_iterator targetIter = mZones.find(target);
    if (targetIter == mZones.end()) {
        LOGE("Target zone '" << target << "' not found");
        result->setError(api::ERROR_INVALID_ID, "Unknown proxy call target");
        return;
    }

    Zone& targetZone = *targetIter->second;
    targetZone.proxyCallAsync(targetBusName,
                                   targetObjectPath,
                                   targetInterface,
                                   targetMethod,
                                   parameters,
                                   asyncResultCallback);
}

void ZonesManager::handleGetZoneDbuses(dbus::MethodResultBuilder::Pointer result)
{
    std::vector<GVariant*> entries;
    for (auto& zone : mZones) {
        GVariant* zoneId = g_variant_new_string(zone.first.c_str());
        GVariant* dbusAddress = g_variant_new_string(zone.second->getDbusAddress().c_str());
        GVariant* entry = g_variant_new_dict_entry(zoneId, dbusAddress);
        entries.push_back(entry);
    }
    GVariant* dict = g_variant_new_array(G_VARIANT_TYPE("{ss}"), entries.data(), entries.size());
    result->set(g_variant_new("(*)", dict));
}

void ZonesManager::handleDbusStateChanged(const std::string& zoneId,
                                               const std::string& dbusAddress)
{
    mHostConnection.signalZoneDbusState(zoneId, dbusAddress);
}

void ZonesManager::handleGetZoneIdsCall(dbus::MethodResultBuilder::Pointer result)
{
    std::vector<GVariant*> zoneIds;
    for(auto& zone: mZones){
        zoneIds.push_back(g_variant_new_string(zone.first.c_str()));
    }

    GVariant* array = g_variant_new_array(G_VARIANT_TYPE("s"),
                                          zoneIds.data(),
                                          zoneIds.size());
    result->set(g_variant_new("(*)", array));
}

void ZonesManager::handleGetActiveZoneIdCall(dbus::MethodResultBuilder::Pointer result)
{
    LOGI("GetActiveZoneId call");
    if (!mConfig.foregroundId.empty() && mZones[mConfig.foregroundId]->isRunning()){
        result->set(g_variant_new("(s)", mConfig.foregroundId.c_str()));
    } else {
        result->set(g_variant_new("(s)", ""));
    }
}

void ZonesManager::handleGetZoneInfoCall(const std::string& id,
                                                   dbus::MethodResultBuilder::Pointer result)
{
    LOGI("GetZoneInfo call");
    if (mZones.count(id) == 0) {
        LOGE("No zone with id=" << id);
        result->setError(api::ERROR_INVALID_ID, "No such zone id");
        return;
    }
    const auto& zone = mZones[id];
    const char* state;
    //TODO: Use the lookup map.
    if (zone->isRunning()) {
        state = "RUNNING";
    } else if (zone->isStopped()) {
        state = "STOPPED";
    } else if (zone->isPaused()) {
        state = "FROZEN";
    } else {
        LOGE("Unrecognized state of zone id=" << id);
        result->setError(api::ERROR_INTERNAL, "Unrecognized state of zone");
        return;
    }
    const auto zonePath = boost::filesystem::absolute(id, mConfig.zonesPath);
    const auto rootfsDir = boost::filesystem::path("rootfs");
    const auto rootfsPath = zonePath / rootfsDir;

    result->set(g_variant_new("((siss))",
                              id.c_str(),
                              zone->getVT(),
                              state,
                              rootfsPath.string().c_str()));
}

void ZonesManager::handleDeclareFileCall(const std::string& zone,
                                              const int32_t& type,
                                              const std::string& path,
                                              const int32_t& flags,
                                              const int32_t& mode,
                                              dbus::MethodResultBuilder::Pointer result)
{
    LOGI("DeclareFile call");
    try {
        mZones.at(zone)->declareFile(type, path, flags, mode);
        result->setVoid();
    } catch (const std::out_of_range&) {
        LOGE("No zone with id=" << zone);
        result->setError(api::ERROR_INVALID_ID, "No such zone id");
    } catch (const config::ConfigException& ex) {
        LOGE("Can't declare file: " << ex.what());
        result->setError(api::ERROR_INTERNAL, "Internal error");
    }
}

void ZonesManager::handleDeclareMountCall(const std::string& source,
                                               const std::string& zone,
                                               const std::string& target,
                                               const std::string& type,
                                               const uint64_t& flags,
                                               const std::string& data,
                                               dbus::MethodResultBuilder::Pointer result)
{
    LOGI("DeclareMount call");
    try {
        mZones.at(zone)->declareMount(source, target, type, flags, data);
        result->setVoid();
    } catch (const std::out_of_range&) {
        LOGE("No zone with id=" << zone);
        result->setError(api::ERROR_INVALID_ID, "No such zone id");
    } catch (const config::ConfigException& ex) {
        LOGE("Can't declare mount: " << ex.what());
        result->setError(api::ERROR_INTERNAL, "Internal error");
    }
}

void ZonesManager::handleDeclareLinkCall(const std::string& source,
                                              const std::string& zone,
                                              const std::string& target,
                                              dbus::MethodResultBuilder::Pointer result)
{
    LOGI("DeclareLink call");
    try {
        mZones.at(zone)->declareLink(source, target);
        result->setVoid();
    } catch (const std::out_of_range&) {
        LOGE("No zone with id=" << zone);
        result->setError(api::ERROR_INVALID_ID, "No such zone id");
    } catch (const config::ConfigException& ex) {
        LOGE("Can't declare link: " << ex.what());
        result->setError(api::ERROR_INTERNAL, "Internal error");
    }
}

void ZonesManager::handleSetActiveZoneCall(const std::string& id,
                                                     dbus::MethodResultBuilder::Pointer result)
{
    LOGI("SetActiveZone call; Id=" << id );
    auto zone = mZones.find(id);
    if (zone == mZones.end()){
        LOGE("No zone with id=" << id );
        result->setError(api::ERROR_INVALID_ID, "No such zone id");
        return;
    }

    if (zone->second->isStopped()){
        LOGE("Could not activate a stopped zone");
        result->setError(api::host::ERROR_ZONE_STOPPED,
                         "Could not activate a stopped zone");
        return;
    }

    focus(id);
    result->setVoid();
}


void ZonesManager::generateNewConfig(const std::string& id,
                                          const std::string& templatePath,
                                          const std::string& resultPath)
{
    namespace fs = boost::filesystem;

    std::string resultFileDir = utils::dirName(resultPath);
    if (!fs::exists(resultFileDir)) {
        if (!utils::createEmptyDir(resultFileDir)) {
            LOGE("Unable to create directory for new config.");
            throw ZoneOperationException("Unable to create directory for new config.");
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
        throw ZoneOperationException("Failed to read template config file.");
    }

    std::string resultConfig = boost::regex_replace(config, ZONE_NAME_REGEX, id);

    // generate third IP octet for network config
    // TODO change algorithm after implementing removeZone
    std::string thirdOctetStr = std::to_string(ZONE_IP_BASE_THIRD_OCTET + mZones.size() + 1);
    LOGD("IP third octet: " << thirdOctetStr);
    resultConfig = boost::regex_replace(resultConfig, ZONE_IP_THIRD_OCTET_REGEX, thirdOctetStr);

    if (!utils::saveFileContent(resultPath, resultConfig)) {
        LOGE("Faield to save new config file.");
        throw ZoneOperationException("Failed to save new config file.");
    }

    // restrict new config file so that only owner (vasum) can write it
    fs::permissions(resultPath, fs::perms::owner_all |
                                fs::perms::group_read |
                                fs::perms::others_read);
}

void ZonesManager::handleCreateZoneCall(const std::string& id,
                                                  dbus::MethodResultBuilder::Pointer result)
{
    if (id.empty()) {
        LOGE("Failed to add zone - invalid name.");
        result->setError(api::ERROR_INVALID_ID, "Invalid name");
        return;
    }

    LOGI("Creating zone " << id);

    // TODO: This solution is temporary. It utilizes direct access to config files when creating new
    // zones. Update this handler when config database will appear.
    namespace fs = boost::filesystem;

    // check if zone does not exist
    if (mZones.find(id) != mZones.end()) {
        LOGE("Cannot create " << id << " zone - already exists!");
        result->setError(api::ERROR_INVALID_ID, "Already exists");
        return;
    }

    const std::string zonePathStr = utils::createFilePath(mConfig.zonesPath, "/", id, "/");

    // copy zone image if config contains path to image
    LOGT("Image path: " << mConfig.zoneImagePath);
    if (!mConfig.zoneImagePath.empty()) {
        auto copyImageContentsWrapper = std::bind(&utils::copyImageContents,
                                                  mConfig.zoneImagePath,
                                                  zonePathStr);

        if (!utils::launchAsRoot(copyImageContentsWrapper)) {
            LOGE("Failed to copy zone image.");
            result->setError(api::ERROR_INTERNAL, "Failed to copy zone image.");
            return;
        }
    }

    // generate paths to new configuration files
    std::string baseDir = utils::dirName(mConfigPath);
    std::string configDir = utils::getAbsolutePath(mConfig.zoneNewConfigPrefix, baseDir);
    std::string templateDir = utils::getAbsolutePath(mConfig.zoneTemplatePath, baseDir);

    std::string configPath = utils::createFilePath(templateDir, "/", ZONE_TEMPLATE_CONFIG_PATH);
    std::string newConfigPath = utils::createFilePath(configDir, "/zones/", id + ".conf");

    auto removeAllWrapper = [](const std::string& path) -> bool {
        try {
            LOGD("Removing copied data");
            fs::remove_all(fs::path(path));
        } catch(const std::exception& e) {
            LOGW("Failed to remove data: " << boost::diagnostic_information(e));
        }
        return true;
    };

    try {
        LOGI("Generating config from " << configPath << " to " << newConfigPath);
        generateNewConfig(id, configPath, newConfigPath);

    } catch (VasumException& e) {
        LOGE("Generate config failed: " << e.what());
        utils::launchAsRoot(std::bind(removeAllWrapper, zonePathStr));
        result->setError(api::ERROR_INTERNAL, "Failed to generate config");
        return;
    }

    LOGT("Creating new zone");
    try {
        createZone(newConfigPath);
    } catch (VasumException& e) {
        LOGE("Creating new zone failed: " << e.what());
        utils::launchAsRoot(std::bind(removeAllWrapper, zonePathStr));
        result->setError(api::ERROR_INTERNAL, "Failed to create zone");
        return;
    }

    auto resultCallback = [this, id, result](bool succeeded) {
        if (succeeded) {
            focus(id);
            result->setVoid();
        } else {
            LOGE("Failed to start zone.");
            // TODO removeZone
            result->setError(api::ERROR_INTERNAL, "Failed to start zone");
        }
    };
    mZones[id]->startAsync(resultCallback);
}

void ZonesManager::handleDestroyZoneCall(const std::string& id,
                                                   dbus::MethodResultBuilder::Pointer result)
{
    if (mZones.find(id) == mZones.end()) {
        LOGE("Failed to destroy zone - no such zone id: " << id);
        result->setError(api::ERROR_INVALID_ID, "No such zone id");
        return;
    }

    LOGI("Destroying zone " << id);

    auto destroyer = [id, result, this] {
        try {
            destroyZone(id);
        } catch (const VasumException& e) {
            LOGE("Error during zone destruction: " << e.what());
            result->setError(api::ERROR_INTERNAL, "Failed to destroy zone");
            return;
        }
        result->setVoid();
    };

    std::thread thread(destroyer);
    thread.detach(); //TODO fix it
}

void ZonesManager::handleLockZoneCall(const std::string& id,
                                                dbus::MethodResultBuilder::Pointer result)
{
    LOGI("LockZone call; Id=" << id );
    auto iter = mZones.find(id);
    if (iter == mZones.end()) {
        LOGE("Failed to lock zone - no such zone id: " << id);
        result->setError(api::ERROR_INVALID_ID, "No such zone id");
        return;
    }

    auto& zone = *iter->second;
    if (!zone.isRunning()) {
        LOGE("Zone id=" << id << " is not running.");
        result->setError(api::ERROR_INVALID_STATE, "Zone is not running");
        return;
    }

    LOGT("Lock zone");
    try {
        zone.suspend();
    } catch (ZoneOperationException& e) {
        LOGE(e.what());
        result->setError(api::ERROR_INTERNAL, e.what());
        return;
    }

    result->setVoid();
}

void ZonesManager::handleUnlockZoneCall(const std::string& id,
                                                  dbus::MethodResultBuilder::Pointer result)
{
    LOGI("UnlockZone call; Id=" << id );
    auto iter = mZones.find(id);
    if (iter == mZones.end()) {
        LOGE("Failed to unlock zone - no such zone id: " << id);
        result->setError(api::ERROR_INVALID_ID, "No such zone id");
        return;
    }

    auto& zone = *iter->second;
    if (!zone.isPaused()) {
        LOGE("Zone id=" << id << " is not paused.");
        result->setError(api::ERROR_INVALID_STATE, "Zone is not paused");
        return;
    }

    LOGT("Unlock zone");
    try {
        zone.resume();
    } catch (ZoneOperationException& e) {
        LOGE(e.what());
        result->setError(api::ERROR_INTERNAL, e.what());
        return;
    }

    result->setVoid();
}

} // namespace vasum
