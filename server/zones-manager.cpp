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
#include "utils/vt.hpp"
#include "api/messages.hpp"

#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <cassert>
#include <string>
#include <climits>
#include <cctype>
#include <set>


namespace vasum {


namespace {


const std::string HOST_ID = "host";
const std::string ENABLED_FILE_NAME = "enabled";

const boost::regex ZONE_NAME_REGEX("~NAME~");
const boost::regex ZONE_IP_THIRD_OCTET_REGEX("~IP~");

const unsigned int ZONE_IP_BASE_THIRD_OCTET = 100;

const std::vector<std::string> prohibitedZonesNames{
    ENABLED_FILE_NAME,
    "lxc-monitord.log"
};

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

bool isalnum(const std::string& str)
{
    for (const auto& c : str) {
        if (!std::isalnum(c)) {
            return false;
        }
    }
    return true;
}

void cleanUpUnknownsFromRoot(const boost::filesystem::path& zonesPath,
                             const std::vector<std::string>& zoneIds,
                             bool dryRun)
{
    namespace fs =  boost::filesystem;
    const auto end = fs::directory_iterator();

    std::set<std::string> knowns(zoneIds.begin(), zoneIds.end());
    knowns.insert(prohibitedZonesNames.begin(), prohibitedZonesNames.end());

    // Remove all directories that start with '.'
    for (auto zoneDir = fs::directory_iterator(zonesPath); zoneDir != end; ++zoneDir) {
        if (zoneDir->path().filename().string()[0] ==  '.') {
            if (!dryRun) {
                fs::remove_all(zoneDir->path());
                LOGI("Remove directory entry: " << *zoneDir);
            } else {
                LOGI("Remove directory entry (dry run): " << *zoneDir);
            }
        }
    }

    for (auto zoneDir = fs::directory_iterator(zonesPath); zoneDir != end; ++zoneDir) {
        const auto zoneIt = knowns.find(zoneDir->path().filename().string());
        if (zoneIt == knowns.end()) {
            if (!dryRun) {
                const std::string filename = '.' + zoneDir->path().filename().string();
                fs::path newName = zoneDir->path().parent_path() / filename;

                fs::rename(zoneDir->path(), newName);
                fs::remove_all(newName);
                LOGI("Remove directory entry: " << *zoneDir);
            } else {
                LOGI("Remove directory entry (dry run): " << *zoneDir);
            }
        }
    }
}

} // namespace


ZonesManager::ZonesManager(ipc::epoll::EventPoll& eventPoll, const std::string& configPath)
    : mIsRunning(true)
    , mWorker(utils::Worker::create())
    , mDetachOnExit(false)
    , mExclusiveIDLock(INVALID_CONNECTION_ID)
    , mHostIPCConnection(eventPoll, this)
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

    if (mConfig.inputConfig.enabled) {
        LOGI("Registering input monitor [" << mConfig.inputConfig.device.c_str() << "]");
        mSwitchingSequenceMonitor.reset(new InputMonitor(eventPoll, mConfig.inputConfig, this));
    }
}

ZonesManager::~ZonesManager()
{
    LOGD("Destroying ZonesManager object...");
    stop(true);
}

void ZonesManager::start()
{
    Lock lock(mMutex);

    LOGD("Starting ZonesManager");

    mIsRunning = true;

    cleanUpUnknownsFromRoot(mConfig.zonesPath, mDynamicConfig.zoneIds, !mConfig.cleanUpZonesPath);

#ifdef DBUS_CONNECTION
    using namespace std::placeholders;
    mProxyCallPolicy.reset(new ProxyCallPolicy(mConfig.proxyCallRules));
    mHostDbusConnection.setProxyCallCallback(std::bind(&ZonesManager::handleProxyCall,
                                                       this, HOST_ID, _1, _2, _3, _4, _5, _6, _7));
#endif //DBUS_CONNECTION

    for (const auto& zoneId : mDynamicConfig.zoneIds) {
        insertZone(zoneId, getTemplatePathForExistingZone(zoneId));
    }

    updateDefaultId();

    LOGD("ZonesManager object instantiated");

    if (mConfig.inputConfig.enabled) {
        LOGI("Starting input monitor ");
        mSwitchingSequenceMonitor->start();
    }

    // After everything's initialized start to respond to clients' requests
    mHostIPCConnection.start();
}

void ZonesManager::stop(bool wait)
{
    Lock lock(mMutex);
    LOGD("Stopping ZonesManager");

    if(!mIsRunning) {
        return;
    }

    if (!mDetachOnExit) {
        try {
            shutdownAll();
        } catch (ServerException&) {
            LOGE("Failed to shutdown all of the zones");
        }
    }

    // wait for all tasks to complete
    mWorker.reset();
    mHostIPCConnection.stop(wait);
    if (mConfig.inputConfig.enabled) {
        LOGI("Stopping input monitor ");
        mSwitchingSequenceMonitor->stop();
    }
    mIsRunning = false;
}

bool ZonesManager::isRunning()
{
    Lock lock(mMutex);
    return mIsRunning ||  mHostIPCConnection.isRunning();
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

void ZonesManager::tryAddTask(const utils::Worker::Task& task, api::MethodResultBuilder::Pointer result, bool wait)
{
    {
        Lock lock(mExclusiveIDMutex);

        if (mExclusiveIDLock != INVALID_CONNECTION_ID &&
            mExclusiveIDLock != result->getID()) {
            result->setError(api::ERROR_QUEUE, "Queue is locked by another client");
            return;
        }
    }

    if (wait) {
        mWorker->addTaskAndWait(task);
    } else {
        mWorker->addTask(task);
    }
}

void ZonesManager::destroyZone(const std::string& zoneId)
{
    Lock lock(mMutex);

    auto iter = findZone(zoneId);
    if (iter == mZones.end()) {
        const std::string msg = "Failed to destroy zone " + zoneId + ": no such zone";
        LOGE(msg);
        throw InvalidZoneIdException(msg);
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
            utils::activateVT(mConfig.hostVT);
            mActiveZoneId.clear();
        }
        return;
    }

    Zone& zoneToFocus = get(iter);
    const std::string& idToFocus = zoneToFocus.getId();

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
        LOGW("Active zone " << mActiveZoneId << " is not running any more!");
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

void ZonesManager::disconnectedCallback(const std::string& id)
{
    LOGD("Client Disconnected: " << id);

    {
        Lock lock(mExclusiveIDMutex);

        if (mExclusiveIDLock == id) {
            mExclusiveIDLock = INVALID_CONNECTION_ID;
        }
    }
}

void ZonesManager::handleSwitchToDefaultCall(const std::string& /*caller*/,
                                             api::MethodResultBuilder::Pointer result)
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
        result->setVoid();
    };

    tryAddTask(handler, result, true);
}

void ZonesManager::handleCreateFileCall(const api::CreateFileIn& request,
                                        api::MethodResultBuilder::Pointer result)
{
    auto handler = [&, this] {
        LOGI("CreateFile call");

        Lock lock(mMutex);

        auto srcIter = findZone(request.id);
        if (srcIter == mZones.end()) {
            LOGE("Zone '" << request.id << "' not found");
            result->setError(api::ERROR_INVALID_ID, "Requested Zone was not found.");
            return;
        }
        Zone& srcZone = get(srcIter);

        auto retValue = std::make_shared<api::CreateFileOut>();
        try {
            retValue->fd = srcZone.createFile(request.path, request.flags, request.mode);
        } catch(ZoneOperationException& e) {
            result->setError(api::ERROR_CREATE_FILE_FAILED, "Unable to create file");
            return;
        }

        result->set(retValue);
    };

    tryAddTask(handler, result, true);
}

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

    // This call cannot be locked by lock/unlock queue
    mWorker->addTaskAndWait(handler);
}
#endif //DBUS_CONNECTION

void ZonesManager::handleLockQueueCall(api::MethodResultBuilder::Pointer result)
{
    Lock lock(mExclusiveIDMutex);
    std::string id = result->getID();

    LOGI("Lock Queue: " << id);

    if (mExclusiveIDLock == id) {
        result->setError(api::ERROR_QUEUE, "Queue already locked");
        return;
    }

    if (mExclusiveIDLock != INVALID_CONNECTION_ID) {
        result->setError(api::ERROR_QUEUE, "Queue locked by another connection");
        return;
    }

    mExclusiveIDLock = id;
    result->setVoid();
}

void ZonesManager::handleUnlockQueueCall(api::MethodResultBuilder::Pointer result)
{
    Lock lock(mExclusiveIDMutex);
    std::string id = result->getID();

    LOGI("Unlock Queue: " << id);

    if (mExclusiveIDLock == INVALID_CONNECTION_ID) {
        result->setError(api::ERROR_QUEUE, "Queue not locked");
        return;
    }

    if (mExclusiveIDLock != id) {
        result->setError(api::ERROR_QUEUE, "Queue locked by another connection");
        return;
    }

    mExclusiveIDLock = INVALID_CONNECTION_ID;
    result->setVoid();
}

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

    // This call cannot be locked by lock/unlock queue
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

    // This call cannot be locked by lock/unlock queue
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

    // This call cannot be locked by lock/unlock queue
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

    tryAddTask(handler, result, true);
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

    tryAddTask(handler, result, true);
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

    tryAddTask(handler, result, true);
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

    tryAddTask(handler, result, true);
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

    tryAddTask(handler, result, true);
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

    tryAddTask(handler, result, true);
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

    tryAddTask(handler, result, true);
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

    tryAddTask(handler, result, true);
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

    tryAddTask(handler, result, true);
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

    tryAddTask(handler, result, true);
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

    tryAddTask(handler, result, true);
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

    tryAddTask(handler, result, true);
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

    tryAddTask(handler, result, true);
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

    tryAddTask(handler, result, true);
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

    if (dynamicConfig.vt >= 0) {
        // generate first free VT number
        const int freeVT = getVTForNewZone();
        LOGD("VT number: " << freeVT);
        dynamicConfig.vt = freeVT;

        if (!dynamicConfig.ipv4Gateway.empty() && !dynamicConfig.ipv4.empty()) {
            // generate third IP octet for network config
            std::string thirdOctetStr = std::to_string(ZONE_IP_BASE_THIRD_OCTET + freeVT);
            LOGD("IP third octet: " << thirdOctetStr);
            dynamicConfig.ipv4Gateway = boost::regex_replace(dynamicConfig.ipv4Gateway,
                                                             ZONE_IP_THIRD_OCTET_REGEX,
                                                             thirdOctetStr);
            dynamicConfig.ipv4 = boost::regex_replace(dynamicConfig.ipv4,
                                                      ZONE_IP_THIRD_OCTET_REGEX,
                                                      thirdOctetStr);
        }
    }

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
        const std::string msg = "No free VT for zone";
        LOGE(msg);
        throw ZoneOperationException(msg);
    }
    // return the smallest
    return *candidates.begin();
}

void ZonesManager::createZone(const std::string& id,
                              const std::string& templateName)
{
    if (id.empty() || !isalnum(id)) {
        const std::string msg = "Failed to add zone - invalid name.";
        LOGE(msg);
        throw InvalidZoneIdException(msg);
    }

    if (find(prohibitedZonesNames.begin(), prohibitedZonesNames.end(), id) != prohibitedZonesNames.end()) {
        const std::string msg = "Cannot create " + id + " zone - name is not allowed!";
        LOGE(msg);
        throw InvalidZoneIdException(msg);
    }

    LOGI("Creating zone " << id);

    Lock lock(mMutex);

    // TODO: This solution is temporary. It utilizes direct access to config files when creating new
    // zones. Update this handler when config database will appear.
    namespace fs = boost::filesystem;

    // check if zone does not exist
    if (findZone(id) != mZones.end()) {
        const std::string msg = "Cannot create " + id + " zone - already exists!";
        LOGE(msg);
        throw InvalidZoneIdException(msg);
    }

    if (fs::exists(fs::path(mConfig.zonesPath) / id)) {
        const std::string msg = "Cannot create " + id + " zone - file system already exists!";
        LOGE(msg);
        throw InvalidZoneIdException(msg);
    }

    const std::string zonePathStr = utils::createFilePath(mConfig.zonesPath, id, "/");

    // copy zone image if config contains path to image
    LOGT("Image path: " << mConfig.zoneImagePath);
    if (!mConfig.zoneImagePath.empty()) {
        auto copyImageContentsWrapper = std::bind(&utils::copyImageContents,
                                                  mConfig.zoneImagePath,
                                                  zonePathStr);

        if (!utils::launchAsRoot(copyImageContentsWrapper)) {
            const std::string msg = "Failed to copy zone image.";
            LOGE(msg);
            throw ZoneOperationException(msg);
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
        throw;
    }

    LOGT("Creating new zone");
    try {
        insertZone(id, zoneTemplatePath);
    } catch (std::runtime_error& e) {
        LOGE("Creating new zone failed: " << e.what());
        utils::launchAsRoot(std::bind(removeAllWrapper, zonePathStr));
        throw;
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
            result->setError(api::ERROR_INVALID_ID, e.what());
        } catch (const std::exception& e) {
            result->setError(api::ERROR_INTERNAL, e.what());
        }
    };

    tryAddTask(creator, result, true);
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

    tryAddTask(destroyer, result, false);
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

    tryAddTask(shutdown, result, false);
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
    tryAddTask(startAsync, result, false);
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

    tryAddTask(handler, result, true);
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

    tryAddTask(handler, result, true);
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

    tryAddTask(handler, result, true);
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

    tryAddTask(handler, result, true);
}

void ZonesManager::handleCleanUpZonesRootCall(api::MethodResultBuilder::Pointer result)
{
    auto handler = [&, this] {
        LOGI("CleanUpZonesRoot call");
        try {
            std::vector<std::string> zonesIds;
            Lock lock(mMutex);
            for (const auto& zone : mZones) {
                zonesIds.push_back(zone->getId());
            }
            cleanUpUnknownsFromRoot(mConfig.zonesPath, zonesIds, false);
        } catch (const std::exception& e) {
            result->setError(api::ERROR_INTERNAL, e.what());
        }
        result->setVoid();
    };

    tryAddTask(handler, result, true);
}

} // namespace vasum
