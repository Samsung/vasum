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
#include "lxc/cgroup.hpp"
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

const std::string DB_PREFIX = "daemon";
const std::string HOST_ID = "host";
const std::string ENABLED_FILE_NAME = "enabled";

const boost::regex ZONE_NAME_REGEX("~NAME~");
const boost::regex ZONE_IP_THIRD_OCTET_REGEX("~IP~");
const boost::regex ZONE_VT_REGEX("~VT~");

const unsigned int ZONE_IP_BASE_THIRD_OCTET = 100;
const unsigned int ZONE_VT_BASE = 1;

std::string getConfigName(const std::string& zoneId)
{
    return "zones/" + zoneId + ".conf";
}

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

std::vector<std::unique_ptr<Zone>>::iterator find(std::vector<std::unique_ptr<Zone>>& zones,
                                                  const std::string& id)
{
    auto equalId = [&id](const std::unique_ptr<Zone>& zone) {
        return zone->getId() == id;
    };
    return std::find_if(zones.begin(), zones.end(), equalId);
}

Zone& get(std::vector<std::unique_ptr<Zone>>::iterator iter)
{
    return **iter;
}

Zone& at(std::vector<std::unique_ptr<Zone>>& zones, const std::string& id)
{
    auto iter = find(zones, id);
    if (iter == zones.end()) {
        throw std::out_of_range("id not found");
    }
    return get(iter);
}

bool zoneIsRunning(const std::unique_ptr<Zone>& zone) {
    return zone->isRunning();
}

} // namespace

ZonesManager::ZonesManager(const std::string& configPath)
    : mWorker(utils::Worker::create())
    , mDetachOnExit(false)
{
    LOGD("Instantiating ZonesManager object...");

    config::loadFromJsonFile(configPath, mConfig);
    config::loadFromKVStoreWithJsonFile(mConfig.dbPath, configPath, mDynamicConfig, DB_PREFIX);

    mProxyCallPolicy.reset(new ProxyCallPolicy(mConfig.proxyCallRules));

    using namespace std::placeholders;
    mHostConnection.setProxyCallCallback(bind(&ZonesManager::handleProxyCall,
                                              this, HOST_ID, _1, _2, _3, _4, _5, _6, _7));

    mHostConnection.setGetZoneDbusesCallback(bind(&ZonesManager::handleGetZoneDbuses,
                                                  this, _1));

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

    mHostConnection.setShutdownZoneCallback(bind(&ZonesManager::handleShutdownZoneCall,
                                                 this, _1, _2));

    mHostConnection.setStartZoneCallback(bind(&ZonesManager::handleStartZoneCall,
                                              this, _1, _2));

    mHostConnection.setLockZoneCallback(bind(&ZonesManager::handleLockZoneCall,
                                             this, _1, _2));

    mHostConnection.setUnlockZoneCallback(bind(&ZonesManager::handleUnlockZoneCall,
                                               this, _1, _2));

    mHostConnection.setGrantDeviceCallback(bind(&ZonesManager::handleGrantDeviceCall,
                                                this, _1, _2, _3, _4));

    mHostConnection.setRevokeDeviceCallback(bind(&ZonesManager::handleRevokeDeviceCall,
                                                 this, _1, _2, _3));

    for (const auto& zoneConfig : mDynamicConfig.zoneConfigs) {
        createZone(utils::createFilePath(mConfig.zoneNewConfigPrefix, zoneConfig));
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
    // wait for all tasks to complete
    mWorker.reset();

    LOGD("ZonesManager object destroyed");
}

void ZonesManager::saveDynamicConfig()
{
    config::saveToKVStore(mConfig.dbPath, mDynamicConfig, DB_PREFIX);
}

void ZonesManager::updateDefaultId()
{
    // TODO add an api to change defaultId
    if (mZones.empty() && mDynamicConfig.defaultId.empty()) {
        LOGT("Keep empty defaultId");
        return;
    }
    if (find(mZones, mDynamicConfig.defaultId) != mZones.end()) {
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

void ZonesManager::createZone(const std::string& zoneConfigPath)
{
    LOGT("Creating Zone " << zoneConfigPath);
    std::unique_ptr<Zone> zone(new Zone(mWorker->createSubWorker(),
                                        mConfig.zonesPath,
                                        zoneConfigPath,
                                        mConfig.dbPath,
                                        mConfig.lxcTemplatePrefix,
                                        mConfig.runMountPointPrefix));
    const std::string id = zone->getId();
    if (id == HOST_ID) {
        throw ZoneOperationException("Cannot use reserved zone ID");
    }

    using namespace std::placeholders;
    zone->setNotifyActiveZoneCallback(bind(&ZonesManager::notifyActiveZoneHandler,
                                           this, id, _1, _2));

    zone->setDisplayOffCallback(bind(&ZonesManager::displayOffHandler,
                                     this, id));

    zone->setFileMoveRequestCallback(bind(&ZonesManager::handleZoneMoveFileRequest,
                                          this, id, _1, _2, _3));

    zone->setProxyCallCallback(bind(&ZonesManager::handleProxyCall,
                                    this, id, _1, _2, _3, _4, _5, _6, _7));

    zone->setDbusStateChangedCallback(bind(&ZonesManager::handleDbusStateChanged,
                                           this, id, _1));

    Lock lock(mMutex);

    if (find(mZones, id) != mZones.end()) {
        throw ZoneOperationException("Zone already exists");
    }
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

    auto iter = find(mZones, zoneId);
    if (iter == mZones.end()) {
        LOGE("Failed to destroy zone " << zoneId << ": no such zone");
        throw ZoneOperationException("No such zone");
    }

    get(iter).setDestroyOnExit();
    mZones.erase(iter);

    if (mZones.empty()) {
        if (!utils::removeFile(utils::createFilePath(mConfig.zonesPath, ENABLED_FILE_NAME))) {
            LOGE("Failed to remove enabled file.");
        }
    }

    // update dynamic config
    remove(mDynamicConfig.zoneConfigs, getConfigName(zoneId));
    saveDynamicConfig();
    updateDefaultId();

    refocus();
}

void ZonesManager::focus(const std::string& zoneId)
{
    Lock lock(mMutex);
    auto iter = find(mZones, zoneId);
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

    Zone& zone = get(iter);
    std::string zoneId = zone.getId();

    if (zoneId == mActiveZoneId) {
        return;
    }

    if (!zone.isRunning()) {
        LOGE("Can't focus not running zone " << zoneId);
        assert(false);
        return;
    }

    LOGI("Focus to: " << zone.getId());

    if (!zone.activateVT()) {
        LOGE("Failed to activate zones VT");
        return;
    }

    for (auto& zone : mZones) {
        std::string id = zone->getId();
        if (id == zoneId) {
            LOGD(id << ": being sent to foreground");
            zone->goForeground();
        } else {
            LOGD(id << ": being sent to background");
            zone->goBackground();
        }
    }
    mActiveZoneId = zoneId;
}

void ZonesManager::refocus()
{
    // assume mutex is locked

    // check if refocus is required
    auto oldIter = find(mZones, mActiveZoneId);
    if (oldIter != mZones.end() && get(oldIter).isRunning()) {
        return;
    }

    // try to refocus to defaultId
    auto iter = find(mZones, mDynamicConfig.defaultId);
    if (iter != mZones.end() && get(iter).isRunning()) {
        // focus to default
        focusInternal(iter);
    } else {
        // focus to any running or to host if not found
        auto iter = std::find_if(mZones.begin(), mZones.end(), zoneIsRunning);
        if (iter == mZones.end()) {
            focusInternal(mZones.end());
        } else {
            focusInternal(iter);
        }
    }
}

void ZonesManager::startAll()
{
    LOGI("Starting all zones");

    Lock lock(mMutex);

    for (auto& zone : mZones) {
        zone->start();
    }

    refocus();
}

void ZonesManager::stopAll()
{
    LOGI("Stopping all zones");

    Lock lock(mMutex);

    for (auto& zone : mZones) {
        zone->stop();
    }

    refocus();
}

bool ZonesManager::isPaused(const std::string& zoneId)
{
    Lock lock(mMutex);

    auto iter = find(mZones, zoneId);
    if (iter == mZones.end()) {
        LOGE("No such zone id: " << zoneId);
        throw ZoneOperationException("No such zone");
    }

    return get(iter).isPaused();
}

bool ZonesManager::isRunning(const std::string& zoneId)
{
    Lock lock(mMutex);

    auto iter = find(mZones, zoneId);
    if (iter == mZones.end()) {
        LOGE("No such zone id: " << zoneId);
        throw ZoneOperationException("No such zone");
    }
    return get(iter).isRunning();
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
    auto iter = find(mZones, mActiveZoneId);
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
    auto current = find(mZones, mActiveZoneId);
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

void ZonesManager::notifyActiveZoneHandler(const std::string& caller,
                                           const std::string& application,
                                           const std::string& message)
{
    LOGI("notifyActiveZoneHandler(" << caller << ", " << application << ", " << message
         << ") called");

    Lock lock(mMutex);

    try {
        auto iter = getRunningForegroundZoneIterator();
        if (iter != mZones.end() && caller != get(iter).getId()) {
            get(iter).sendNotification(caller, application, message);
        }
    } catch(const VasumException&) {
        LOGE("Notification from " << caller << " hasn't been sent");
    }
}

void ZonesManager::displayOffHandler(const std::string& /*caller*/)
{
    // get config of currently set zone and switch if switchToDefaultAfterTimeout is true
    Lock lock(mMutex);

    auto activeIter = find(mZones, mActiveZoneId);
    auto defaultIter = find(mZones, mDynamicConfig.defaultId);

    if (activeIter != mZones.end() &&
        defaultIter != mZones.end() &&
        get(activeIter).isSwitchToDefaultAfterTimeoutAllowed() &&
        get(defaultIter).isRunning()) {

        LOGI("Switching to default zone " << mDynamicConfig.defaultId);
        focusInternal(defaultIter);
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

    Lock lock(mMutex);

    auto srcIter = find(mZones, srcZoneId);
    if (srcIter == mZones.end()) {
        LOGE("Source zone '" << srcZoneId << "' not found");
        return;
    }
    Zone& srcZone = get(srcIter);

    auto dstIter = find(mZones, dstZoneId);
    if (dstIter == mZones.end()) {
        LOGE("Destination zone '" << dstZoneId << "' not found");
        result->set(g_variant_new("(s)", api::zone::FILE_MOVE_DESTINATION_NOT_FOUND.c_str()));
        return;
    }
    Zone& dstContanier = get(dstIter);

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

    Lock lock(mMutex);

    auto targetIter = find(mZones, target);
    if (targetIter == mZones.end()) {
        LOGE("Target zone '" << target << "' not found");
        result->setError(api::ERROR_INVALID_ID, "Unknown proxy call target");
        return;
    }

    Zone& targetZone = get(targetIter);
    targetZone.proxyCallAsync(targetBusName,
                                   targetObjectPath,
                                   targetInterface,
                                   targetMethod,
                                   parameters,
                                   asyncResultCallback);
}

void ZonesManager::handleGetZoneDbuses(dbus::MethodResultBuilder::Pointer result)
{
    Lock lock(mMutex);

    std::vector<GVariant*> entries;
    for (auto& zone : mZones) {
        GVariant* zoneId = g_variant_new_string(zone->getId().c_str());
        GVariant* dbusAddress = g_variant_new_string(zone->getDbusAddress().c_str());
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
    Lock lock(mMutex);

    std::vector<GVariant*> zoneIds;
    for(auto& zone: mZones){
        zoneIds.push_back(g_variant_new_string(zone->getId().c_str()));
    }

    GVariant* array = g_variant_new_array(G_VARIANT_TYPE("s"),
                                          zoneIds.data(),
                                          zoneIds.size());
    result->set(g_variant_new("(*)", array));
}

void ZonesManager::handleGetActiveZoneIdCall(dbus::MethodResultBuilder::Pointer result)
{
    LOGI("GetActiveZoneId call");

    std::string id = getRunningForegroundZoneId();
    result->set(g_variant_new("(s)", id.c_str()));
}

void ZonesManager::handleGetZoneInfoCall(const std::string& id,
                                         dbus::MethodResultBuilder::Pointer result)
{
    LOGI("GetZoneInfo call");

    Lock lock(mMutex);

    auto iter = find(mZones, id);
    if (iter == mZones.end()) {
        LOGE("No zone with id=" << id);
        result->setError(api::ERROR_INVALID_ID, "No such zone id");
        return;
    }
    Zone& zone = get(iter);
    const char* state;

    if (zone.isRunning()) {
        state = "RUNNING";
    } else if (zone.isStopped()) {
        state = "STOPPED";
    } else if (zone.isPaused()) {
        state = "FROZEN";
    } else {
        LOGE("Unrecognized state of zone id=" << id);
        result->setError(api::ERROR_INTERNAL, "Unrecognized state of zone");
        return;
    }

    result->set(g_variant_new("((siss))",
                              id.c_str(),
                              zone.getVT(),
                              state,
                              zone.getRootPath().c_str()));
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
        Lock lock(mMutex);

        at(mZones, zone).declareFile(type, path, flags, mode);
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
        Lock lock(mMutex);

        at(mZones, zone).declareMount(source, target, type, flags, data);
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
        Lock lock(mMutex);

        at(mZones, zone).declareLink(source, target);
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

    Lock lock(mMutex);

    auto iter = find(mZones, id);
    if (iter == mZones.end()){
        LOGE("No zone with id=" << id );
        result->setError(api::ERROR_INVALID_ID, "No such zone id");
        return;
    }

    if (!get(iter).isRunning()){
        LOGE("Could not activate stopped or paused zone");
        result->setError(api::host::ERROR_ZONE_NOT_RUNNING,
                         "Could not activate stopped or paused zone");
        return;
    }

    focusInternal(iter);
    result->setVoid();
}


void ZonesManager::generateNewConfig(const std::string& id,
                                     const std::string& templatePath,
                                     const std::string& resultPath)
{
    namespace fs = boost::filesystem;

    if (fs::exists(resultPath)) {
        LOGT(resultPath << " already exists, removing");
        fs::remove(resultPath);
    } else {
        std::string resultFileDir = utils::dirName(resultPath);
        if (!utils::createDirs(resultFileDir, fs::perms::owner_all |
                                              fs::perms::group_read | fs::perms::group_exe |
                                              fs::perms::others_read | fs::perms::others_exe)) {
            LOGE("Unable to create directory for new config.");
            throw ZoneOperationException("Unable to create directory for new config.");
        }
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

    // generate first free VT number
    // TODO change algorithm after implementing removeZone
    std::string freeVT = std::to_string(ZONE_VT_BASE + mZones.size() + 1);
    LOGD("VT number: " << freeVT);
    resultConfig = boost::regex_replace(resultConfig, ZONE_VT_REGEX, freeVT);

    if (!utils::saveFileContent(resultPath, resultConfig)) {
        LOGE("Faield to save new config file.");
        throw ZoneOperationException("Failed to save new config file.");
    }

    // restrict new config file so that only owner (vasum) can write it
    fs::permissions(resultPath, fs::perms::owner_read | fs::perms::owner_write |
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

    Lock lock(mMutex);

    // TODO: This solution is temporary. It utilizes direct access to config files when creating new
    // zones. Update this handler when config database will appear.
    namespace fs = boost::filesystem;

    // check if zone does not exist
    if (find(mZones, id) != mZones.end()) {
        LOGE("Cannot create " << id << " zone - already exists!");
        result->setError(api::ERROR_INVALID_ID, "Already exists");
        return;
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
            result->setError(api::ERROR_INTERNAL, "Failed to copy zone image.");
            return;
        }
    }

    // generate paths to new configuration files
    std::string newConfigName = getConfigName(id);
    std::string newConfigPath = utils::createFilePath(mConfig.zoneNewConfigPrefix, newConfigName);

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
        LOGI("Generating config from " << mConfig.zoneTemplatePath << " to " << newConfigPath);
        generateNewConfig(id, mConfig.zoneTemplatePath, newConfigPath);

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

    mDynamicConfig.zoneConfigs.push_back(newConfigName);
    saveDynamicConfig();
    updateDefaultId();

    result->setVoid();
}

void ZonesManager::handleDestroyZoneCall(const std::string& id,
                                         dbus::MethodResultBuilder::Pointer result)
{
    auto destroyer = [id, result, this] {
        try {
            LOGI("Destroying zone " << id);

            destroyZone(id);
        } catch (const ZoneOperationException& e) {
            LOGE("Failed to destroy zone - no such zone id: " << id);
            result->setError(api::ERROR_INVALID_ID, "No such zone id");
        } catch (const VasumException& e) {
            LOGE("Error during zone destruction: " << e.what());
            result->setError(api::ERROR_INTERNAL, "Failed to destroy zone");
            return;
        }
        result->setVoid();
    };

    mWorker->addTask(destroyer);
}

void ZonesManager::handleShutdownZoneCall(const std::string& id,
                                          dbus::MethodResultBuilder::Pointer result)
{
    LOGI("ShutdownZone call; Id=" << id );

    auto shutdown = [id, result, this] {
        try {
            LOGT("Shutdown zone " << id);

            Lock lock(mMutex);
            auto iter = find(mZones, id);
            if (iter == mZones.end()) {
                LOGE("Failed to shutdown zone - no such zone id: " << id);
                result->setError(api::ERROR_INVALID_ID, "No such zone id");
                return;
            }
            get(iter).stop();
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

void ZonesManager::handleStartZoneCall(const std::string& id,
                                       dbus::MethodResultBuilder::Pointer result)
{
    LOGI("StartZone call; Id=" << id );

    auto startAsync = [this, id, result]() {
        try {
            LOGT("Start zone " << id );

            Lock lock(mMutex);
            auto iter = find(mZones, id);
            if (iter == mZones.end()) {
                LOGE("Failed to start zone - no such zone id: " << id);
                result->setError(api::ERROR_INVALID_ID, "No such zone id");
                return;
            }
            get(iter).start();
            focusInternal(iter);
            result->setVoid();
        } catch (const std::exception& e) {
            LOGE(id << ": failed to start: " << e.what());
            result->setError(api::ERROR_INTERNAL, "Failed to start zone");
        }
    };
    mWorker->addTask(startAsync);
}

void ZonesManager::handleLockZoneCall(const std::string& id,
                                      dbus::MethodResultBuilder::Pointer result)
{
    LOGI("LockZone call; Id=" << id );

    Lock lock(mMutex);

    auto iter = find(mZones, id);
    if (iter == mZones.end()) {
        LOGE("Failed to lock zone - no such zone id: " << id);
        result->setError(api::ERROR_INVALID_ID, "No such zone id");
        return;
    }

    Zone& zone = get(iter);
    if (!zone.isRunning()) {
        LOGE("Zone id=" << id << " is not running.");
        result->setError(api::ERROR_INVALID_STATE, "Zone is not running");
        return;
    }

    LOGT("Lock zone");
    try {
        zone.suspend();
        refocus();
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

    Lock lock(mMutex);

    auto iter = find(mZones, id);
    if (iter == mZones.end()) {
        LOGE("Failed to unlock zone - no such zone id: " << id);
        result->setError(api::ERROR_INVALID_ID, "No such zone id");
        return;
    }

    Zone& zone = get(iter);
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

void ZonesManager::handleGrantDeviceCall(const std::string& id,
                                         const std::string& device,
                                         uint32_t flags,
                                         dbus::MethodResultBuilder::Pointer result)
{
    LOGI("GrantDevice call; id=" << id << "; dev=" << device);

    Lock lock(mMutex);

    auto iter = find(mZones, id);
    if (iter == mZones.end()) {
        LOGE("Failed to grant device - no such zone id: " << id);
        result->setError(api::ERROR_INVALID_ID, "No such zone id");
        return;
    }

    Zone& zone = get(iter);
    if (!zone.isRunning() && !zone.isPaused()) {
        LOGE("Zone id=" << id << " is not running");
        result->setError(api::ERROR_INVALID_STATE, "Zone is not running");
        return;
    }

    std::string devicePath = "/dev/" + device;

    if (!lxc::isDevice(devicePath)) {
        LOGE("Failed to grant device - cannot acces device: " << device);
        result->setError(api::ERROR_FORBIDDEN, "Cannot access device");
        return;
    }

    // assume device node is created inside zone
    if (!lxc::setDeviceAccess(id, devicePath, true, flags)) {
        LOGE("Failed to grant device: " << device << " for zone: " << id);
        result->setError(api::ERROR_INTERNAL, "Cannot grant device");
        return;
    }

    result->setVoid();
}

void ZonesManager::handleRevokeDeviceCall(const std::string& id,
                                          const std::string& device,
                                          dbus::MethodResultBuilder::Pointer result)
{
    LOGI("RevokeDevice call; id=" << id << "; dev=" << device);

    Lock lock(mMutex);

    auto iter = find(mZones, id);
    if (iter == mZones.end()) {
        LOGE("Failed to revoke device - no such zone id: " << id);
        result->setError(api::ERROR_INVALID_ID, "No such zone id");
        return;
    }

    Zone& zone = get(iter);
    if (!zone.isRunning() && !zone.isPaused()) {
        LOGE("Zone id=" << id << " is not running");
        result->setError(api::ERROR_INVALID_STATE, "Zone is not running");
        return;
    }
    std::string devicePath = "/dev/" + device;

    if (!lxc::isDevice(devicePath)) {
        LOGE("Failed to revoke device - cannot acces device: " << device);
        result->setError(api::ERROR_FORBIDDEN, "Cannot access device");
        return;
    }

    if (!lxc::setDeviceAccess(id, devicePath, false, 0)) {
        LOGE("Failed to revoke device: " << device << " for zone: " << id);
        result->setError(api::ERROR_INTERNAL, "Cannot revoke device");
        return;
    }

    result->setVoid();
}

} // namespace vasum
