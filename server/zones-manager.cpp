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

#include "common-definitions.hpp"
#include "dynamic-config-scheme.hpp"
#include "zones-manager.hpp"
#include "lxc/cgroup.hpp"
#include "exception.hpp"

#include "utils/paths.hpp"
#include "logger/logger.hpp"
#include "config/manager.hpp"
#include "dbus/exception.hpp"
#include "utils/fs.hpp"
#include "utils/img.hpp"
#include "utils/environment.hpp"
#include "api/messages.hpp"

#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <cassert>
#include <string>
#include <climits>


namespace vasum {


namespace {

#ifdef ZONE_CONNECTION
bool regexMatchVector(const std::string& str, const std::vector<boost::regex>& v)
{
    for (const boost::regex& toMatch : v) {
        if (boost::regex_match(str, toMatch)) {
            return true;
        }
    }

    return false;
}
#endif

const std::string HOST_ID = "host";
const std::string ENABLED_FILE_NAME = "enabled";

const boost::regex ZONE_NAME_REGEX("~NAME~");
const boost::regex ZONE_IP_THIRD_OCTET_REGEX("~IP~");

const unsigned int ZONE_IP_BASE_THIRD_OCTET = 100;

template<typename T>
void remove(std::vector<T>& v, const T& item)
{
    // erase-remove idiom, ask google for explanation
    v.erase(std::remove(v.begin(), v.end(), item), v.end());
}

template<typename Iter, typename Predicate>
Iter circularFindNext(Iter begin, Iter end, Iter current, Predicate pred)
{
    if (begin == end || current == end) {
        return end;
    }
    for (Iter next = current;;) {
        ++ next;
        if (next == end) {
            next = begin;
        }
        if (next == current) {
            return end;
        }
        if (pred(*next)) {
            return next;
        }
    }
}

Zone& get(std::vector<std::unique_ptr<Zone>>::iterator iter)
{
    return **iter;
}

bool zoneIsRunning(const std::unique_ptr<Zone>& zone) {
    return zone->isRunning();
}

} // namespace


ZonesManager::ZonesManager(const std::string& configPath)
    : mWorker(utils::Worker::create())
    , mHostIPCConnection(this)
    , mDetachOnExit(false)
#ifdef DBUS_CONNECTION
    , mHostDbusConnection(this)
#endif
{
    LOGD("Instantiating ZonesManager object...");

    config::loadFromJsonFile(configPath, mConfig);
    config::loadFromKVStoreWithJsonFile(mConfig.dbPath,
                                        configPath,
                                        mDynamicConfig,
                                        getVasumDbPrefix());

#ifdef DBUS_CONNECTION
    using namespace std::placeholders;
    mProxyCallPolicy.reset(new ProxyCallPolicy(mConfig.proxyCallRules));
    mHostDbusConnection.setProxyCallCallback(bind(&ZonesManager::handleProxyCall,
                                                  this, HOST_ID, _1, _2, _3, _4, _5, _6, _7));
#endif //DBUS_CONNECTION

    for (const auto& zoneId : mDynamicConfig.zoneIds) {
        insertZone(zoneId, getTemplatePathForExistingZone(zoneId));
    }

    updateDefaultId();

    LOGD("ZonesManager object instantiated");

    if (mConfig.inputConfig.enabled) {
        LOGI("Registering input monitor [" << mConfig.inputConfig.device.c_str() << "]");
        mSwitchingSequenceMonitor.reset(
            new InputMonitor(mConfig.inputConfig,
                             std::bind(&ZonesManager::switchingSequenceMonitorNotify,
                                       this)));
    }

    // After everything's initialized start to respond to clients' requests
    mHostIPCConnection.start();
}

ZonesManager::~ZonesManager()
{
    LOGD("Destroying ZonesManager object...");

    if (!mDetachOnExit) {
        try {
            shutdownAll();
        } catch (ServerException&) {
            LOGE("Failed to shutdown all of the zones");
        }
    }
    // wait for all tasks to complete
    mWorker.reset();

    LOGD("ZonesManager object destroyed");
}

ZonesManager::Zones::iterator ZonesManager::findZone(const std::string& id)
{
    return std::find_if(mZones.begin(), mZones.end(), [&id](const std::unique_ptr<Zone>& zone) {
        return zone->getId() == id;
    });
}

Zone& ZonesManager::getZone(const std::string& id)
{
    auto iter = findZone(id);
    if (iter == mZones.end()) {
        throw InvalidZoneIdException("Zone id not found");
    }
    return get(iter);
}

void ZonesManager::saveDynamicConfig()
{
    config::saveToKVStore(mConfig.dbPath, mDynamicConfig, getVasumDbPrefix());
}

void ZonesManager::updateDefaultId()
{
    // TODO add an api to change defaultId
    if (mZones.empty() && mDynamicConfig.defaultId.empty()) {
        LOGT("Keep empty defaultId");
        return;
    }
    if (findZone(mDynamicConfig.defaultId) != mZones.end()) {
        LOGT("Keep " << mDynamicConfig.defaultId << " as defaultId");
        return;
    }

    // update
    if (mZones.empty()) {
        mDynamicConfig.defaultId.clear();
        LOGD("DefaultId cleared");
    } else {
        mDynamicConfig.defaultId = mZones.front()->getId();
        LOGD("DefaultId changed to " << mDynamicConfig.defaultId);
    }
    saveDynamicConfig();
}

std::string ZonesManager::getTemplatePathForExistingZone(const std::string& id)
{
    ZoneTemplatePathConfig config;
    config::loadFromKVStore(mConfig.dbPath, config, getZoneDbPrefix(id));
    return config.zoneTemplatePath;
}

void ZonesManager::insertZone(const std::string& zoneId, const std::string& zoneTemplatePath)
{
    if (zoneId == HOST_ID) {
        throw InvalidZoneIdException("Cannot use reserved zone ID");
    }
    if (findZone(zoneId) != mZones.end()) {
        throw InvalidZoneIdException("Zone already exists");
    }

    LOGT("Creating Zone " << zoneId);
    std::unique_ptr<Zone> zone(new Zone(zoneId,
                                        mConfig.zonesPath,
                                        zoneTemplatePath,
                                        mConfig.dbPath,
                                        mConfig.zoneTemplateDir,
                                        mConfig.runMountPointPrefix));

    mZones.push_back(std::move(zone));

    // after zone is created successfully, put a file informing that zones are enabled
    if (mZones.size() == 1) {
        if (!utils::saveFileContent(
                utils::createFilePath(mConfig.zonesPath, ENABLED_FILE_NAME), "")) {
            throw ZoneOperationException(ENABLED_FILE_NAME + ": cannot create.");
        }
    }
}

void ZonesManager::destroyZone(const std::string& zoneId)
{
    Lock lock(mMutex);

    auto iter = findZone(zoneId);
    if (iter == mZones.end()) {
        LOGE("Failed to destroy zone " << zoneId << ": no such zone");
        throw InvalidZoneIdException("No such zone");
    }

    get(iter).setDestroyOnExit();
    mZones.erase(iter);

    if (mZones.empty()) {
        if (!utils::removeFile(utils::createFilePath(mConfig.zonesPath, ENABLED_FILE_NAME))) {
            LOGE("Failed to remove enabled file.");
        }
    }

    // update dynamic config
    remove(mDynamicConfig.zoneIds, zoneId);
    saveDynamicConfig();
    updateDefaultId();

    refocus();
}

void ZonesManager::focus(const std::string& zoneId)
{
    Lock lock(mMutex);
    auto iter = findZone(zoneId);
    focusInternal(iter);
}

void ZonesManager::focusInternal(Zones::iterator iter)
{
    // assume mutex is locked
    if (iter == mZones.end()) {
        if (!mActiveZoneId.empty()) {
            LOGI("Focus to: host");
            // give back the focus to the host
            // TODO switch to host vt
            mActiveZoneId.clear();
        }
        return;
    }

    Zone& zoneToFocus = get(iter);
    const std::string idToFocus = zoneToFocus.getId();

    if (idToFocus == mActiveZoneId) {
        return;
    }

    if (!zoneToFocus.isRunning()) {
        LOGE("Can't focus not running zone " << idToFocus);
        assert(false);
        return;
    }

    LOGI("Focus to: " << idToFocus);

    if (!zoneToFocus.activateVT()) {
        LOGE("Failed to activate zones VT");
        return;
    }

    for (auto& zone : mZones) {
        if (zone->isRunning()) {
            std::string id = zone->getId();
            if (id == idToFocus) {
                LOGD(id << ": being sent to foreground");
                zone->goForeground();
            } else {
                LOGD(id << ": being sent to background");
                zone->goBackground();
            }
        }
    }
    mActiveZoneId = idToFocus;
}

void ZonesManager::refocus()
{
    // assume mutex is locked

    // check if refocus is required
    auto oldIter = findZone(mActiveZoneId);
    if (oldIter != mZones.end() && get(oldIter).isRunning()) {
        return;
    }

    // try to refocus to defaultId
    auto iter = findZone(mDynamicConfig.defaultId);
    if (iter == mZones.end() || !get(iter).isRunning()) {
        // focus to any running or to host if not found
        iter = std::find_if(mZones.begin(), mZones.end(), zoneIsRunning);
    }
    focusInternal(iter);
}

void ZonesManager::restoreAll()
{
    LOGI("Restoring all zones");

    Lock lock(mMutex);

    for (auto& zone : mZones) {
        zone->restore();
    }

    refocus();
}

void ZonesManager::shutdownAll()
{
    LOGI("Stopping all zones");

    Lock lock(mMutex);

    for (auto& zone : mZones) {
        zone->stop(false);
    }

    refocus();
}

bool ZonesManager::isPaused(const std::string& zoneId)
{
    Lock lock(mMutex);
    return getZone(zoneId).isPaused();
}

bool ZonesManager::isRunning(const std::string& zoneId)
{
    Lock lock(mMutex);
    return getZone(zoneId).isRunning();
}

bool ZonesManager::isStopped(const std::string& zoneId)
{
    Lock lock(mMutex);
    return getZone(zoneId).isStopped();
}

std::string ZonesManager::getRunningForegroundZoneId()
{
    Lock lock(mMutex);
    auto iter = getRunningForegroundZoneIterator();
    return iter == mZones.end() ? std::string() : get(iter).getId();
}

std::string ZonesManager::getNextToForegroundZoneId()
{
    Lock lock(mMutex);
    auto iter = getNextToForegroundZoneIterator();
    return iter == mZones.end() ? std::string() : get(iter).getId();
}

ZonesManager::Zones::iterator ZonesManager::getRunningForegroundZoneIterator()
{
    // assume mutex is locked
    if (mActiveZoneId.empty()) {
        return mZones.end();
    }
    auto iter = findZone(mActiveZoneId);
    if (!get(iter).isRunning()) {
        // Can zone change its state by itself?
        // Maybe when it is shut down by itself? TODO check it
        LOGW("Active zone " << mActiveZoneId << " is not running any more!");
        assert(false);
        return mZones.end();
    }
    return iter;
}

ZonesManager::Zones::iterator ZonesManager::getNextToForegroundZoneIterator()
{
    // assume mutex is locked
    auto current = findZone(mActiveZoneId);
    if (current == mZones.end()) {
        // find any running
        return std::find_if(mZones.begin(), mZones.end(), zoneIsRunning);
    } else {
        // find next running
        return circularFindNext(mZones.begin(), mZones.end(), current, zoneIsRunning);
    }
}

void ZonesManager::switchingSequenceMonitorNotify()
{
    LOGI("switchingSequenceMonitorNotify() called");

    Lock lock(mMutex);

    auto next = getNextToForegroundZoneIterator();

    if (next != mZones.end()) {
        focusInternal(next);
    }
}


void ZonesManager::setZonesDetachOnExit()
{
    Lock lock(mMutex);

    mDetachOnExit = true;

    for (auto& zone : mZones) {
        zone->setDetachOnExit();
    }
}

void ZonesManager::handleSwitchToDefaultCall(const std::string& /*caller*/)
{
    auto handler = [&, this] {
        // get config of currently set zone and switch if switchToDefaultAfterTimeout is true
        Lock lock(mMutex);

        auto activeIter = findZone(mActiveZoneId);
        auto defaultIter = findZone(mDynamicConfig.defaultId);

        if (activeIter != mZones.end() &&
            defaultIter != mZones.end() &&
            get(activeIter).isSwitchToDefaultAfterTimeoutAllowed() &&
            get(defaultIter).isRunning()) {

            LOGI("Switching to default zone " << mDynamicConfig.defaultId);
            focusInternal(defaultIter);
        }
    };

    mWorker->addTaskAndWait(handler);
}

#ifdef ZONE_CONNECTION
void ZonesManager::handleNotifyActiveZoneCall(const std::string& caller,
                                              const api::NotifActiveZoneIn& notif,
                                              api::MethodResultBuilder::Pointer result)
{
    auto handler = [&, this] {
        const std::string& application = notif.first;
        const std::string& message = notif.second;
        LOGI("handleNotifyActiveZoneCall(" << caller << ", " << application << ", " << message
             << ") called");

        Lock lock(mMutex);

        try {
            auto iter = getRunningForegroundZoneIterator();
            if (iter != mZones.end() && caller != get(iter).getId()) {
                //XXX:get(iter).sendNotification(caller, application, message);
            }
            result->setVoid();
        } catch (const std::runtime_error&) {
            LOGE("Notification from " << caller << " hasn't been sent");
            result->setError(api::ERROR_INTERNAL, "Notification hasn't been sent");
        }
    };

    mWorker->addTaskAndWait(handler);
}

void ZonesManager::handleFileMoveCall(const std::string& srcZoneId,
                                      const api::FileMoveRequestIn& request,
                                      api::MethodResultBuilder::Pointer result)
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

    auto handler = [&, this] {
        const std::string& dstZoneId = request.first;
        const std::string& path = request.second;
        LOGI("File move requested\n"
             << "src: " << srcZoneId << "\n"
             << "dst: " << dstZoneId << "\n"
             << "path: " << path);

        Lock lock(mMutex);

        auto srcIter = findZone(srcZoneId);
        if (srcIter == mZones.end()) {
            LOGE("Source zone '" << srcZoneId << "' not found");
            return;
        }
        Zone& srcZone = get(srcIter);

        auto status = std::make_shared<api::FileMoveRequestStatus>();

        auto dstIter = findZone(dstZoneId);
        if (dstIter == mZones.end()) {
            LOGE("Destination zone '" << dstZoneId << "' not found");
            status->value = api::FILE_MOVE_DESTINATION_NOT_FOUND;
            result->set(status);
            return;
        }
        Zone& dstContanier = get(dstIter);

        if (srcZoneId == dstZoneId) {
            LOGE("Cannot send a file to yourself");
            status->value = api::FILE_MOVE_WRONG_DESTINATION;
            result->set(status);
            return;
        }

        if (!regexMatchVector(path, srcZone.getPermittedToSend())) {
            LOGE("Source zone has no permissions to send the file: " << path);
            status->value = api::FILE_MOVE_NO_PERMISSIONS_SEND;
            result->set(status);
            return;
        }

        if (!regexMatchVector(path, dstContanier.getPermittedToRecv())) {
            LOGE("Destination zone has no permissions to receive the file: " << path);
            status->value = api::FILE_MOVE_NO_PERMISSIONS_RECEIVE;
            result->set(status);
            return;
        }

        namespace fs = boost::filesystem;
        std::string srcPath = fs::absolute(srcZoneId, mConfig.zonesPath).string() + path;
        std::string dstPath = fs::absolute(dstZoneId, mConfig.zonesPath).string() + path;

        if (!utils::moveFile(srcPath, dstPath)) {
            LOGE("Failed to move the file: " << path);
            status->value = api::FILE_MOVE_FAILED;
            result->set(status);
        } else {
            status->value = api::FILE_MOVE_SUCCEEDED;
            result->set(status);
            try {
                //XXX: dstContanier.sendNotification(srcZoneId, path, api::FILE_MOVE_SUCCEEDED);
            } catch (ServerException&) {
                LOGE("Notification to '" << dstZoneId << "' has not been sent");
            }
        }
    };

    mWorker->addTaskAndWait(handler);
}
#else
void ZonesManager::handleNotifyActiveZoneCall(const std::string& /* caller */,
                                              const api::NotifActiveZoneIn& /*notif*/,
                                              api::MethodResultBuilder::Pointer result)
{
    result->setError(api::ERROR_INTERNAL, "Not implemented");
}

void ZonesManager::handleFileMoveCall(const std::string& /*srcZoneId*/,
                                      const api::FileMoveRequestIn& /*request*/,
                                      api::MethodResultBuilder::Pointer result)
{
    result->setError(api::ERROR_INTERNAL, "Not implemented");
}

#endif /* ZONE_CONNECTION */

#ifdef DBUS_CONNECTION
void ZonesManager::handleProxyCall(const std::string& caller,
                                   const std::string& target,
                                   const std::string& targetBusName,
                                   const std::string& targetObjectPath,
                                   const std::string& targetInterface,
                                   const std::string& targetMethod,
                                   GVariant* parameters,
                                   dbus::MethodResultBuilder::Pointer result)
{
    auto handler = [&, this] {
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

        auto asyncResultCallback = [result](dbus::AsyncMethodCallResult & asyncMethodCallResult) {
            try {
                GVariant* targetResult = asyncMethodCallResult.get();
                result->set(g_variant_new("(v)", targetResult));
            } catch (dbus::DbusException& e) {
                result->setError(api::ERROR_FORWARDED, e.what());
            }
        };

        if (target != HOST_ID) {
            result->setError(api::ERROR_INVALID_ID, "Unknown proxy call target");
            return;
        }

        mHostDbusConnection.proxyCallAsync(targetBusName,
                                           targetObjectPath,
                                           targetInterface,
                                           targetMethod,
                                           parameters,
                                           asyncResultCallback);
    };

    mWorker->addTaskAndWait(handler);
}
#endif //DBUS_CONNECTION

void ZonesManager::handleGetZoneIdsCall(api::MethodResultBuilder::Pointer result)
{
    auto handler = [&, this] {
        LOGI("GetZoneIds call");

        Lock lock(mMutex);

        auto zoneIds = std::make_shared<api::ZoneIds>();
        for (const auto& zone : mZones) {
            zoneIds->values.push_back(zone->getId());
        }

        result->set(zoneIds);
    };

    mWorker->addTaskAndWait(handler);
}

void ZonesManager::handleGetActiveZoneIdCall(api::MethodResultBuilder::Pointer result)
{
    auto handler = [&, this] {
        LOGI("GetActiveZoneId call");

        auto zoneId = std::make_shared<api::ZoneId>();
        zoneId->value = getRunningForegroundZoneId();
        result->set(zoneId);
    };

    mWorker->addTaskAndWait(handler);
}

void ZonesManager::handleGetZoneInfoCall(const api::ZoneId& zoneId,
                                         api::MethodResultBuilder::Pointer result)
{
    auto handler = [&, this] {
        LOGI("GetZoneInfo call");

        Lock lock(mMutex);

        auto iter = findZone(zoneId.value);
        if (iter == mZones.end()) {
            LOGE("No zone with id=" << zoneId.value);
            result->setError(api::ERROR_INVALID_ID, "No such zone id");
            return;
        }

        Zone& zone = get(iter);
        auto zoneInfo = std::make_shared<api::ZoneInfoOut>();

        if (zone.isRunning()) {
            zoneInfo->state = "RUNNING";
        } else if (zone.isStopped()) {
            zoneInfo->state = "STOPPED";
        } else if (zone.isPaused()) {
            zoneInfo->state = "FROZEN";
        } else {
            LOGE("Unrecognized state of zone id=" << zoneId.value);
            result->setError(api::ERROR_INTERNAL, "Unrecognized state of zone");
            return;
        }

        zoneInfo->id = zone.getId();
        zoneInfo->vt = zone.getVT();
        zoneInfo->rootPath = zone.getRootPath();
        result->set(zoneInfo);
    };

    mWorker->addTaskAndWait(handler);
}

void ZonesManager::handleSetNetdevAttrsCall(const api::SetNetDevAttrsIn& data,
                                            api::MethodResultBuilder::Pointer result)
{
    auto handler = [&, this] {
        LOGI("SetNetdevAttrs call");

        try {
            Lock lock(mMutex);

            // TODO: Use vector<StringPair> instead of tuples
            std::vector<std::tuple<std::string, std::string>> attrsAsTuples;
            for(const auto& entry: data.attrs){
                attrsAsTuples.push_back(std::make_tuple(entry.first, entry.second));
            }

            getZone(data.id).setNetdevAttrs(data.netDev, attrsAsTuples);
            result->setVoid();
        } catch (const InvalidZoneIdException&) {
            LOGE("No zone with id=" << data.id);
            result->setError(api::ERROR_INVALID_ID, "No such zone id");
        } catch (const std::runtime_error& ex) {
            LOGE("Can't set attributes: " << ex.what());
            result->setError(api::ERROR_INTERNAL, ex.what());
        }
    };

    mWorker->addTaskAndWait(handler);
}

void ZonesManager::handleGetNetdevAttrsCall(const api::GetNetDevAttrsIn& data,
                                            api::MethodResultBuilder::Pointer result)
{
    auto handler = [&, this] {
        LOGI("GetNetdevAttrs call");

        try {
            Lock lock(mMutex);
            auto netDevAttrs = std::make_shared<api::GetNetDevAttrs>();
            const auto attrs = getZone(data.first).getNetdevAttrs(data.second);

            for (size_t i = 0; i < attrs.size(); ++i) {
                netDevAttrs->values.push_back({std::get<0>(attrs[i]), std::get<1>(attrs[i])});
            }
            result->set(netDevAttrs);
        } catch (const InvalidZoneIdException&) {
            LOGE("No zone with id=" << data.first);
            result->setError(api::ERROR_INVALID_ID, "No such zone id");
        } catch (const std::runtime_error& ex) {
            LOGE("Can't set attributes: " << ex.what());
            result->setError(api::ERROR_INTERNAL, ex.what());
        }
    };

    mWorker->addTaskAndWait(handler);
}

void ZonesManager::handleGetNetdevListCall(const api::ZoneId& zoneId,
                                           api::MethodResultBuilder::Pointer result)
{
    auto handler = [&, this] {
        LOGI("GetNetdevList call");

        try {
            Lock lock(mMutex);
            auto netDevList = std::make_shared<api::NetDevList>();
            netDevList->values = getZone(zoneId.value).getNetdevList();
            result->set(netDevList);
        } catch (const InvalidZoneIdException&) {
            LOGE("No zone with id=" << zoneId.value);
            result->setError(api::ERROR_INVALID_ID, "No such zone id");
        } catch (const std::runtime_error& ex) {
            LOGE("Can't set attributes: " << ex.what());
            result->setError(api::ERROR_INTERNAL, ex.what());
        }
    };

    mWorker->addTaskAndWait(handler);
}

void ZonesManager::handleCreateNetdevVethCall(const api::CreateNetDevVethIn& data,
                                              api::MethodResultBuilder::Pointer result)
{
    auto handler = [&, this] {
        LOGI("CreateNetdevVeth call");

        try {
            Lock lock(mMutex);

            getZone(data.id).createNetdevVeth(data.zoneDev, data.hostDev);
            result->setVoid();
        } catch (const InvalidZoneIdException&) {
            LOGE("No zone with id=" << data.id);
            result->setError(api::ERROR_INVALID_ID, "No such zone id");
        } catch (const std::runtime_error& ex) {
            LOGE("Can't create veth: " << ex.what());
            result->setError(api::ERROR_INTERNAL, ex.what());
        }
    };

    mWorker->addTaskAndWait(handler);
}

void ZonesManager::handleCreateNetdevMacvlanCall(const api::CreateNetDevMacvlanIn& data,
                                                 api::MethodResultBuilder::Pointer result)
{
    auto handler = [&, this] {
        LOGI("CreateNetdevMacvlan call");

        try {
            Lock lock(mMutex);
            getZone(data.id).createNetdevMacvlan(data.zoneDev, data.hostDev, data.mode);
            result->setVoid();
        } catch (const InvalidZoneIdException&) {
            LOGE("No zone with id=" << data.id);
            result->setError(api::ERROR_INVALID_ID, "No such zone id");
        } catch (const std::runtime_error& ex) {
            LOGE("Can't create macvlan: " << ex.what());
            result->setError(api::ERROR_INTERNAL, ex.what());
        }
    };

    mWorker->addTaskAndWait(handler);
}

void ZonesManager::handleCreateNetdevPhysCall(const api::CreateNetDevPhysIn& data,
                                              api::MethodResultBuilder::Pointer result)
{
    auto handler = [&, this] {
        LOGI("CreateNetdevPhys call");

        try {
            Lock lock(mMutex);

            getZone(data.first).moveNetdev(data.second);
            result->setVoid();
        } catch (const InvalidZoneIdException&) {
            LOGE("No zone with id=" << data.first);
            result->setError(api::ERROR_INVALID_ID, "No such zone id");
        } catch (const std::runtime_error& ex) {
            LOGE("Can't create netdev: " << ex.what());
            result->setError(api::ERROR_INTERNAL, ex.what());
        }
    };

    mWorker->addTaskAndWait(handler);
}

void ZonesManager::handleDestroyNetdevCall(const api::DestroyNetDevIn& data,
                                           api::MethodResultBuilder::Pointer result)
{
    auto handler = [&, this] {
        LOGI("DestroyNetdev call");

        try {
            Lock lock(mMutex);

            getZone(data.first).destroyNetdev(data.second);
            result->setVoid();
        } catch (const InvalidZoneIdException&) {
            LOGE("No zone with id=" << data.first);
            result->setError(api::ERROR_INVALID_ID, "No such zone id");
        } catch (const std::runtime_error& ex) {
            LOGE("Can't create netdev: " << ex.what());
            result->setError(api::ERROR_INTERNAL, ex.what());
        }
    };

    mWorker->addTaskAndWait(handler);
}

void ZonesManager::handleDeleteNetdevIpAddressCall(const api::DeleteNetdevIpAddressIn& data,
                                                   api::MethodResultBuilder::Pointer result)
{
    auto handler = [&, this] {
        LOGI("DelNetdevIpAddress call");

        try {
            Lock lock(mMutex);
            getZone(data.zone).deleteNetdevIpAddress(data.netdev, data.ip);
            result->setVoid();
        } catch (const InvalidZoneIdException&) {
            LOGE("No zone with id=" << data.zone);
            result->setError(api::ERROR_INVALID_ID, "No such zone id");
        } catch (const std::runtime_error& ex) {
            LOGE("Can't delete address: " << ex.what());
            result->setError(api::ERROR_INTERNAL, ex.what());
        }
    };

    mWorker->addTaskAndWait(handler);
}

void ZonesManager::handleDeclareFileCall(const api::DeclareFileIn& data,
                                         api::MethodResultBuilder::Pointer result)
{
    auto handler = [&, this] {
        LOGI("DeclareFile call");

        try {
            Lock lock(mMutex);
            auto declaration = std::make_shared<api::Declaration>();
            declaration->value = getZone(data.zone).declareFile(data.type, data.path, data.flags, data.mode);
            result->set(declaration);
        } catch (const InvalidZoneIdException&) {
            LOGE("No zone with id=" << data.zone);
            result->setError(api::ERROR_INVALID_ID, "No such zone id");
        } catch (const config::ConfigException& ex) {
            LOGE("Can't declare file: " << ex.what());
            result->setError(api::ERROR_INTERNAL, "Internal error");
        }
    };

    mWorker->addTaskAndWait(handler);
}

void ZonesManager::handleDeclareMountCall(const api::DeclareMountIn& data,
                                          api::MethodResultBuilder::Pointer result)
{
    auto handler = [&, this] {
        LOGI("DeclareMount call");

        try {
            Lock lock(mMutex);
            auto declaration = std::make_shared<api::Declaration>();
            declaration->value = getZone(data.zone).declareMount(data.source, data.target, data.type, data.flags, data.data);
            result->set(declaration);
        } catch (const InvalidZoneIdException&) {
            LOGE("No zone with id=" << data.zone);
            result->setError(api::ERROR_INVALID_ID, "No such zone id");
        } catch (const config::ConfigException& ex) {
            LOGE("Can't declare mount: " << ex.what());
            result->setError(api::ERROR_INTERNAL, "Internal error");
        }
    };

    mWorker->addTaskAndWait(handler);
}

void ZonesManager::handleDeclareLinkCall(const api::DeclareLinkIn& data,
                                         api::MethodResultBuilder::Pointer result)
{
    auto handler = [&, this] {
        LOGI("DeclareLink call");

        try {
            Lock lock(mMutex);
            auto declaration = std::make_shared<api::Declaration>();
            declaration->value = getZone(data.zone).declareLink(data.source, data.target);
            result->set(declaration);
        } catch (const InvalidZoneIdException&) {
            LOGE("No zone with id=" << data.zone);
            result->setError(api::ERROR_INVALID_ID, "No such zone id");
        } catch (const config::ConfigException& ex) {
            LOGE("Can't declare link: " << ex.what());
            result->setError(api::ERROR_INTERNAL, "Internal error");
        }
    };

    mWorker->addTaskAndWait(handler);
}

void ZonesManager::handleGetDeclarationsCall(const api::ZoneId& zoneId,
                                             api::MethodResultBuilder::Pointer result)
{
    auto handler = [&, this] {
        LOGI("GetDeclarations call Id=" << zoneId.value);

        try {
            Lock lock(mMutex);
            auto declarations = std::make_shared<api::Declarations>();
            declarations->values = getZone(zoneId.value).getDeclarations();
            result->set(declarations);
        } catch (const InvalidZoneIdException&) {
            LOGE("No zone with id=" << zoneId.value);
            result->setError(api::ERROR_INVALID_ID, "No such zone id");
        } catch (const std::runtime_error& ex) {
            LOGE(ex.what());
            result->setError(api::ERROR_INTERNAL, ex.what());
        }
    };

    mWorker->addTaskAndWait(handler);
}

void ZonesManager::handleRemoveDeclarationCall(const api::RemoveDeclarationIn& data,
                                               api::MethodResultBuilder::Pointer result)
{
    auto handler = [&, this] {
        LOGI("RemoveDeclaration call Id=" << data.first);

        try {
            Lock lock(mMutex);
            getZone(data.first).removeDeclaration(data.second);
            result->setVoid();
        } catch (const InvalidZoneIdException&) {
            LOGE("No zone with id=" << data.first);
            result->setError(api::ERROR_INVALID_ID, "No such zone id");
        } catch (const std::runtime_error& ex) {
            LOGE(ex.what());
            result->setError(api::ERROR_INTERNAL, ex.what());
        }
    };

    mWorker->addTaskAndWait(handler);
}

void ZonesManager::handleSetActiveZoneCall(const api::ZoneId& zoneId,
                                           api::MethodResultBuilder::Pointer result)
{
    auto handler = [&, this] {
        LOGI("SetActiveZone call; Id=" << zoneId.value );

        Lock lock(mMutex);

        auto iter = findZone(zoneId.value);
        if (iter == mZones.end()) {
            LOGE("No zone with id=" << zoneId.value);
            result->setError(api::ERROR_INVALID_ID, "No such zone id");
            return;
        }

        if (!get(iter).isRunning()) {
            LOGE("Could not activate stopped or paused zone");
            result->setError(api::ERROR_ZONE_NOT_RUNNING,
                             "Could not activate stopped or paused zone");
            return;
        }

        focusInternal(iter);
        result->setVoid();
    };

    mWorker->addTaskAndWait(handler);
}


void ZonesManager::generateNewConfig(const std::string& id,
                                     const std::string& templatePath)
{
    const std::string dbPrefix = getZoneDbPrefix(id);
    ZoneDynamicConfig dynamicConfig;
    config::loadFromKVStoreWithJsonFile(mConfig.dbPath, templatePath, dynamicConfig, dbPrefix);

    // update mount point path
    dynamicConfig.runMountPoint = boost::regex_replace(dynamicConfig.runMountPoint,
                                                       ZONE_NAME_REGEX,
                                                       id);

    // generate first free VT number
    const int freeVT = getVTForNewZone();
    LOGD("VT number: " << freeVT);
    dynamicConfig.vt = freeVT;

    // generate third IP octet for network config
    std::string thirdOctetStr = std::to_string(ZONE_IP_BASE_THIRD_OCTET + freeVT);
    LOGD("IP third octet: " << thirdOctetStr);
    dynamicConfig.ipv4Gateway = boost::regex_replace(dynamicConfig.ipv4Gateway,
                                                     ZONE_IP_THIRD_OCTET_REGEX,
                                                     thirdOctetStr);
    dynamicConfig.ipv4 = boost::regex_replace(dynamicConfig.ipv4,
                                              ZONE_IP_THIRD_OCTET_REGEX,
                                              thirdOctetStr);

    // save dynamic config
    config::saveToKVStore(mConfig.dbPath, dynamicConfig, dbPrefix);

    // save zone template path
    ZoneTemplatePathConfig templatePathConfig;
    templatePathConfig.zoneTemplatePath = templatePath;
    config::saveToKVStore(mConfig.dbPath, templatePathConfig, dbPrefix);

}

int ZonesManager::getVTForNewZone()
{
    if (mConfig.availableVTs.empty()) {
        return -1;
    }
    std::set<int> candidates(mConfig.availableVTs.begin(), mConfig.availableVTs.end());
    // exclude all used
    for (auto& zone : mZones) {
        candidates.erase(zone->getVT());
    }
    if (candidates.empty()) {
        LOGE("No free VT for zone");
        throw ZoneOperationException("No free VT for zone");
    }
    // return the smallest
    return *candidates.begin();
}

void ZonesManager::createZone(const std::string& id,
                              const std::string& templateName)
{
    if (id.empty()) { // TODO validate id (no spaces, slashes etc)
        LOGE("Failed to add zone - invalid name.");
        throw InvalidZoneIdException("Invalid name");
    }

    LOGI("Creating zone " << id);

    Lock lock(mMutex);

    // TODO: This solution is temporary. It utilizes direct access to config files when creating new
    // zones. Update this handler when config database will appear.
    namespace fs = boost::filesystem;

    // check if zone does not exist
    if (findZone(id) != mZones.end()) {
        LOGE("Cannot create " << id << " zone - already exists!");
        throw InvalidZoneIdException("Already exists");
    }

    const std::string zonePathStr = utils::createFilePath(mConfig.zonesPath, id, "/");

    // copy zone image if config contains path to image
    LOGT("Image path: " << mConfig.zoneImagePath);
    if (!mConfig.zoneImagePath.empty()) {
        auto copyImageContentsWrapper = std::bind(&utils::copyImageContents,
                                                  mConfig.zoneImagePath,
                                                  zonePathStr);

        if (!utils::launchAsRoot(copyImageContentsWrapper)) {
            LOGE("Failed to copy zone image.");
            throw ZoneOperationException("Failed to copy zone image.");
        }
    }

    auto removeAllWrapper = [](const std::string & path) -> bool {
        try {
            LOGD("Removing copied data");
            fs::remove_all(fs::path(path));
        } catch (const std::exception& e) {
            LOGW("Failed to remove data: " << boost::diagnostic_information(e));
        }
        return true;
    };

    std::string zoneTemplatePath = utils::createFilePath(mConfig.zoneTemplateDir,
                                                         templateName + ".conf");

    try {
        LOGI("Generating config from " << zoneTemplatePath);
        generateNewConfig(id, zoneTemplatePath);
    } catch (std::runtime_error& e) {
        LOGE("Generate config failed: " << e.what());
        utils::launchAsRoot(std::bind(removeAllWrapper, zonePathStr));
        throw e;
    }

    LOGT("Creating new zone");
    try {
        insertZone(id, zoneTemplatePath);
    } catch (std::runtime_error& e) {
        LOGE("Creating new zone failed: " << e.what());
        utils::launchAsRoot(std::bind(removeAllWrapper, zonePathStr));
        throw e;
    }

    mDynamicConfig.zoneIds.push_back(id);
    saveDynamicConfig();
    updateDefaultId();
}

void ZonesManager::handleCreateZoneCall(const api::CreateZoneIn& data,
                                        api::MethodResultBuilder::Pointer result)
{
    auto creator = [&, this] {
        try {
            createZone(data.first, data.second);
            result->setVoid();
        } catch (const InvalidZoneIdException& e) {
            result->setError(api::ERROR_INVALID_ID, "Existing or invalid zone id");
        } catch (const std::runtime_error& e) {
            result->setError(api::ERROR_INTERNAL, "Failed to create zone");
        }
    };

    mWorker->addTaskAndWait(creator);
}

void ZonesManager::handleDestroyZoneCall(const api::ZoneId& zoneId,
                                         api::MethodResultBuilder::Pointer result)
{
    auto destroyer = [=] {
        try {
            LOGI("Destroying zone " << zoneId.value);
            destroyZone(zoneId.value);
        } catch (const InvalidZoneIdException&) {
            LOGE("Failed to destroy zone - no such zone id: " << zoneId.value);
            result->setError(api::ERROR_INVALID_ID, "No such zone id");
        } catch (const std::runtime_error& e) {
            LOGE("Error during zone destruction: " << e.what());
            result->setError(api::ERROR_INTERNAL, "Failed to destroy zone");
            return;
        }
        result->setVoid();
    };

    mWorker->addTask(destroyer);
}

void ZonesManager::handleShutdownZoneCall(const api::ZoneId& zoneId,
                                          api::MethodResultBuilder::Pointer result)
{
    auto shutdown = [=] {
        LOGI("ShutdownZone call; Id=" << zoneId.value);

        try {
            LOGT("Shutdown zone " << zoneId.value);

            Lock lock(mMutex);
            auto iter = findZone(zoneId.value);
            if (iter == mZones.end()) {
                LOGE("Failed to shutdown zone - no such zone id: " << zoneId.value);
                result->setError(api::ERROR_INVALID_ID, "No such zone id");
                return;
            }
            get(iter).stop(true);
            refocus();
            result->setVoid();
        } catch (ZoneOperationException& e) {
            LOGE("Error during zone shutdown: " << e.what());
            result->setError(api::ERROR_INTERNAL, "Failed to shutdown zone");
            return;
        }
    };

    mWorker->addTask(shutdown);
}

void ZonesManager::handleStartZoneCall(const api::ZoneId& zoneId,
                                       api::MethodResultBuilder::Pointer result)
{
    auto startAsync = [=] {
        LOGI("StartZone call; Id=" << zoneId.value);

        try {
            LOGT("Start zone " << zoneId.value);

            Lock lock(mMutex);
            auto iter = findZone(zoneId.value);
            if (iter == mZones.end()) {
                LOGE("Failed to start zone - no such zone id: " << zoneId.value);
                result->setError(api::ERROR_INVALID_ID, "No such zone id");
                return;
            }
            get(iter).start();
            focusInternal(iter);
            result->setVoid();
        } catch (const std::exception& e) {
            LOGE(zoneId.value << ": failed to start: " << e.what());
            result->setError(api::ERROR_INTERNAL, "Failed to start zone");
        }
    };
    mWorker->addTask(startAsync);
}

void ZonesManager::handleLockZoneCall(const api::ZoneId& zoneId,
                                      api::MethodResultBuilder::Pointer result)
{
    auto handler = [&, this] {
        LOGI("LockZone call; Id=" << zoneId.value );

        Lock lock(mMutex);

        auto iter = findZone(zoneId.value);
        if (iter == mZones.end()) {
            LOGE("Failed to lock zone - no such zone id: " << zoneId.value);
            result->setError(api::ERROR_INVALID_ID, "No such zone id");
            return;
        }

        Zone& zone = get(iter);
        if (!zone.isRunning()) {
            LOGE("Zone id=" << zoneId.value << " is not running.");
            result->setError(api::ERROR_INVALID_STATE, "Zone is not running");
            return;
        }

        LOGT("Lock zone");
        try {
            zone.goBackground();// make sure it will be in background after unlock
            zone.suspend();
            refocus();
        } catch (ZoneOperationException& e) {
            LOGE(e.what());
            result->setError(api::ERROR_INTERNAL, e.what());
            return;
        }

        result->setVoid();
    };

    mWorker->addTaskAndWait(handler);
}

void ZonesManager::handleUnlockZoneCall(const api::ZoneId& zoneId,
                                        api::MethodResultBuilder::Pointer result)
{
    auto handler = [&, this] {
        LOGI("UnlockZone call; Id=" << zoneId.value );

        Lock lock(mMutex);

        auto iter = findZone(zoneId.value);
        if (iter == mZones.end()) {
            LOGE("Failed to unlock zone - no such zone id: " << zoneId.value);
            result->setError(api::ERROR_INVALID_ID, "No such zone id");
            return;
        }

        Zone& zone = get(iter);
        if (!zone.isPaused()) {
            LOGE("Zone id=" << zoneId.value << " is not paused.");
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
    };

    mWorker->addTaskAndWait(handler);
}

void ZonesManager::handleGrantDeviceCall(const api::GrantDeviceIn& data,
                                         api::MethodResultBuilder::Pointer result)
{
    auto handler = [&, this] {
        LOGI("GrantDevice call; id=" << data.id << "; dev=" << data.device);

        Lock lock(mMutex);

        auto iter = findZone(data.id);
        if (iter == mZones.end()) {
            LOGE("Failed to grant device - no such zone id: " << data.id);
            result->setError(api::ERROR_INVALID_ID, "No such zone id");
            return;
        }

        Zone& zone = get(iter);
        if (!zone.isRunning() && !zone.isPaused()) {
            LOGE("Zone id=" << data.id << " is not running");
            result->setError(api::ERROR_INVALID_STATE, "Zone is not running");
            return;
        }

        std::string devicePath = "/dev/" + data.device;

        if (!lxc::isDevice(devicePath)) {
            LOGE("Failed to grant device - cannot acces device: " << data.device);
            result->setError(api::ERROR_FORBIDDEN, "Cannot access device");
            return;
        }

        // assume device node is created inside zone
        if (!lxc::setDeviceAccess(data.id, devicePath, true, data.flags)) {
            LOGE("Failed to grant device: " << data.device << " for zone: " << data.id);
            result->setError(api::ERROR_INTERNAL, "Cannot grant device");
            return;
        }

        result->setVoid();
    };

    mWorker->addTaskAndWait(handler);
}

void ZonesManager::handleRevokeDeviceCall(const api::RevokeDeviceIn& data,
                                          api::MethodResultBuilder::Pointer result)
{
    auto handler = [&, this] {
        LOGI("RevokeDevice call; id=" << data.first << "; dev=" << data.second);

        Lock lock(mMutex);

        auto iter = findZone(data.first);
        if (iter == mZones.end()) {
            LOGE("Failed to revoke device - no such zone id: " << data.first);
            result->setError(api::ERROR_INVALID_ID, "No such zone id");
            return;
        }

        Zone& zone = get(iter);
        if (!zone.isRunning() && !zone.isPaused()) {
            LOGE("Zone id=" << data.first << " is not running");
            result->setError(api::ERROR_INVALID_STATE, "Zone is not running");
            return;
        }
        std::string devicePath = "/dev/" + data.second;

        if (!lxc::isDevice(devicePath)) {
            LOGE("Failed to revoke device - cannot acces device: " << data.second);
            result->setError(api::ERROR_FORBIDDEN, "Cannot access device");
            return;
        }

        if (!lxc::setDeviceAccess(data.first, devicePath, false, 0)) {
            LOGE("Failed to revoke device: " << data.second << " for zone: " << data.first);
            result->setError(api::ERROR_INTERNAL, "Cannot revoke device");
            return;
        }

        result->setVoid();
    };

    mWorker->addTaskAndWait(handler);
}

} // namespace vasum
