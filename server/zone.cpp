/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Lukasz Pawelczyk <l.pawelczyk@partner.samsung.com>
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
 * @author  Lukasz Pawelczyk (l.pawelczyk@partner.samsung.com)
 * @brief   Implementation of class for managing one zone
 */

#include "config.hpp"

#include "zone.hpp"
#include "dynamic-config-scheme.hpp"
#include "base-exception.hpp"

#include "logger/logger.hpp"
#include "utils/paths.hpp"
#include "utils/vt.hpp"
#include "config/manager.hpp"

#include <boost/filesystem.hpp>

#include <string>
#include <thread>

namespace vasum {

namespace fs = boost::filesystem;

namespace {

typedef std::lock_guard<std::recursive_mutex> Lock;

// TODO: move constants to the config file when default values are implemented there
const int RECONNECT_RETRIES = 15;
const int RECONNECT_DELAY = 1 * 1000;

const std::string STATE_STOPPED = "stopped";
const std::string STATE_RUNNING = "running";
const std::string STATE_PAUSED = "paused";

} // namespace

Zone::Zone(const utils::Worker::Pointer& worker,
           const std::string& zoneId,
           const std::string& zonesPath,
           const std::string& zoneTemplatePath,
           const std::string& dbPath,
           const std::string& lxcTemplatePrefix,
           const std::string& baseRunMountPointPath)
    : mWorker(worker)
    , mDbPath(dbPath)
{
    const std::string dbPrefix = getZoneDbPrefix(zoneId);
    config::loadFromKVStoreWithJsonFile(dbPath, zoneTemplatePath, mConfig, dbPrefix);
    config::loadFromKVStoreWithJsonFile(dbPath, zoneTemplatePath, mDynamicConfig, dbPrefix);

    for (std::string r: mConfig.permittedToSend) {
        mPermittedToSend.push_back(boost::regex(r));
    }
    for (std::string r: mConfig.permittedToRecv) {
        mPermittedToRecv.push_back(boost::regex(r));
    }

    if (!mDynamicConfig.runMountPoint.empty()) {
        mRunMountPoint = fs::absolute(mDynamicConfig.runMountPoint, baseRunMountPointPath).string();
    }

    mAdmin.reset(new ZoneAdmin(zoneId, zonesPath, lxcTemplatePrefix, mConfig, mDynamicConfig));

    const fs::path zonePath = fs::path(zonesPath) / zoneId;
    mRootPath = (zonePath / fs::path("rootfs")).string();

    mProvision.reset(new ZoneProvision(mRootPath, zoneTemplatePath, dbPath, dbPrefix, mConfig.validLinkPrefixes));
}

Zone::~Zone()
{
    // Make sure all OnNameLostCallbacks get finished and no new will
    // get called before proceeding further. This guarantees no race
    // condition on the reconnect thread.
    {
        Lock lock(mReconnectMutex);
        disconnect();
    }
    // wait for all tasks to complete
    mWorker.reset();
}

const std::vector<boost::regex>& Zone::getPermittedToSend() const
{
    return mPermittedToSend;
}

const std::vector<boost::regex>& Zone::getPermittedToRecv() const
{
    return mPermittedToRecv;
}

const std::string& Zone::getId() const
{
    Lock lock(mReconnectMutex);
    return mAdmin->getId();
}

int Zone::getPrivilege() const
{
    return mConfig.privilege;
}

void Zone::saveDynamicConfig()
{
    config::saveToKVStore(mDbPath, mDynamicConfig, getZoneDbPrefix(getId()));
}

void Zone::updateRequestedState(const std::string& state)
{
    // assume mutex is locked
    if (state != mDynamicConfig.requestedState) {
        LOGT("Set requested state of " << getId() << " to " << state);
        mDynamicConfig.requestedState = state;
        saveDynamicConfig();
    }
}

void Zone::restore()
{
    std::string requestedState;
    {
        Lock lock(mReconnectMutex);
        requestedState = mDynamicConfig.requestedState;
        LOGT("Requested state of " << getId() << ": " << requestedState);
    }

    if (requestedState == STATE_RUNNING) {
        start();
    } else if (requestedState == STATE_STOPPED) {
    } else if (requestedState == STATE_PAUSED) {
        start();
        suspend();
    } else {
        LOGE("Invalid requested state: " << requestedState);
        assert(0 && "invalid requested state");
    }
}

void Zone::start()
{
    Lock lock(mReconnectMutex);
    updateRequestedState(STATE_RUNNING);
    mProvision->start();
    if (mConfig.enableDbusIntegration) {
        mConnectionTransport.reset(new ZoneConnectionTransport(mRunMountPoint));
    }

    mAdmin->start();
    if (mConfig.enableDbusIntegration) {
        // Increase cpu quota before connect, otherwise it'd take ages.
        goForeground();
        connect();
    }
    // refocus in ZonesManager will adjust cpu quota after all
}

void Zone::stop(bool saveState)
{
    Lock lock(mReconnectMutex);
    if (saveState) {
        updateRequestedState(STATE_STOPPED);
    }
    if (mAdmin->isRunning()) {
        // boost stopping
        goForeground();
    }
    disconnect();
    mAdmin->stop();
    mConnectionTransport.reset();
    mProvision->stop();
}

void Zone::connect()
{
    // assume called under reconnect lock
    mDbusAddress = mConnectionTransport->acquireAddress();
    mConnection.reset(new ZoneConnection(mDbusAddress,
                                              std::bind(&Zone::onNameLostCallback, this)));
    if (mNotifyCallback) {
        mConnection->setNotifyActiveZoneCallback(mNotifyCallback);
    }
    if (mDisplayOffCallback) {
        mConnection->setDisplayOffCallback(mDisplayOffCallback);
    }
    if (mFileMoveCallback) {
        mConnection->setFileMoveCallback(mFileMoveCallback);
    }
    if (mProxyCallCallback) {
        mConnection->setProxyCallCallback(mProxyCallCallback);
    }
    if (mDbusStateChangedCallback) {
        mDbusStateChangedCallback(mDbusAddress);
    }
}

void Zone::disconnect()
{
    // assume called under reconnect lock
    if (mConnection) {
        mConnection.reset();
        mDbusAddress.clear();
        if (mDbusStateChangedCallback) {
            // notify about invalid dbusAddress for this zone
            mDbusStateChangedCallback(std::string());
        }
    }
}

std::string Zone::getDbusAddress() const
{
    Lock lock(mReconnectMutex);
    return mDbusAddress;
}

int Zone::getVT() const
{
    Lock lock(mReconnectMutex);
    return mDynamicConfig.vt;
}

std::string Zone::getRootPath() const
{
    return mRootPath;
}

bool Zone::activateVT()
{
    Lock lock(mReconnectMutex);

    if (mDynamicConfig.vt >= 0) {
        return utils::activateVT(mDynamicConfig.vt);
    }

    return true;
}

void Zone::createNetdevVeth(const std::string& zoneDev,
                            const std::string& hostDev)
{
    Lock lock(mReconnectMutex);
    mAdmin->createNetdevVeth(zoneDev, hostDev);
}

void Zone::createNetdevMacvlan(const std::string& zoneDev,
                               const std::string& hostDev,
                               const uint32_t& mode)
{
    Lock lock(mReconnectMutex);
    mAdmin->createNetdevMacvlan(zoneDev, hostDev, mode);
}

void Zone::moveNetdev(const std::string& devId)
{
    Lock lock(mReconnectMutex);
    mAdmin->moveNetdev(devId);
}

void Zone::destroyNetdev(const std::string& devId)
{
    Lock lock(mReconnectMutex);
    mAdmin->destroyNetdev(devId);
}

void Zone::goForeground()
{
    Lock lock(mReconnectMutex);
    mAdmin->setSchedulerLevel(SchedulerLevel::FOREGROUND);
}

void Zone::goBackground()
{
    Lock lock(mReconnectMutex);
    mAdmin->setSchedulerLevel(SchedulerLevel::BACKGROUND);
}

void Zone::setDetachOnExit()
{
    Lock lock(mReconnectMutex);
    mAdmin->setDetachOnExit();
    if (mConnectionTransport) {
        mConnectionTransport->setDetachOnExit();
    }
}

void Zone::setDestroyOnExit()
{
    Lock lock(mReconnectMutex);
    mAdmin->setDestroyOnExit();
}

bool Zone::isRunning()
{
    Lock lock(mReconnectMutex);
    return mAdmin->isRunning();
}

bool Zone::isStopped()
{
    Lock lock(mReconnectMutex);
    return mAdmin->isStopped();
}

void Zone::suspend()
{
    Lock lock(mReconnectMutex);
    mAdmin->suspend();
    updateRequestedState(STATE_PAUSED);
}

void Zone::resume()
{
    Lock lock(mReconnectMutex);
    mAdmin->resume();
    updateRequestedState(STATE_RUNNING);
}

bool Zone::isPaused()
{
    Lock lock(mReconnectMutex);
    return mAdmin->isPaused();
}

bool Zone::isSwitchToDefaultAfterTimeoutAllowed() const
{
    return mConfig.switchToDefaultAfterTimeout;
}

void Zone::onNameLostCallback()
{
    LOGI(getId() << ": A connection to the DBUS server has been lost, reconnecting...");

    mWorker->addTask(std::bind(&Zone::reconnectHandler, this));
}

void Zone::reconnectHandler()
{
    {
        Lock lock(mReconnectMutex);
        disconnect();
    }

    for (int i = 0; i < RECONNECT_RETRIES; ++i) {
        // This sleeps even before the first try to give DBUS some time to come back up
        std::this_thread::sleep_for(std::chrono::milliseconds(RECONNECT_DELAY));

        Lock lock(mReconnectMutex);
        if (isStopped()) {
            LOGI(getId() << ": Has stopped, nothing to reconnect to, bailing out");
            return;
        }

        try {
            LOGT(getId() << ": Reconnect try " << i + 1);
            connect();
            LOGI(getId() << ": Reconnected");
            return;
        } catch (VasumException&) {
            LOGT(getId() << ": Reconnect try " << i + 1 << " has been unsuccessful");
        }
    }

    LOGE(getId() << ": Reconnecting to the DBUS has failed, stopping the zone");
    stop(false);
}

void Zone::setNotifyActiveZoneCallback(const NotifyActiveZoneCallback& callback)
{
    Lock lock(mReconnectMutex);
    mNotifyCallback = callback;
    if (mConnection) {
        mConnection->setNotifyActiveZoneCallback(mNotifyCallback);
    }
}

void Zone::sendNotification(const std::string& zone,
                                 const std::string& application,
                                 const std::string& message)
{
    Lock lock(mReconnectMutex);
    if (mConnection) {
        mConnection->sendNotification(zone, application, message);
    } else {
        LOGE(getId() << ": Can't send notification, no connection to DBUS");
    }
}

void Zone::setDisplayOffCallback(const DisplayOffCallback& callback)
{
    Lock lock(mReconnectMutex);

    mDisplayOffCallback = callback;
    if (mConnection) {
        mConnection->setDisplayOffCallback(callback);
    }
}

void Zone::setFileMoveCallback(const FileMoveCallback& callback)
{
    Lock lock(mReconnectMutex);

    mFileMoveCallback = callback;
    if (mConnection) {
        mConnection->setFileMoveCallback(callback);
    }
}

void Zone::setProxyCallCallback(const ProxyCallCallback& callback)
{
    Lock lock(mReconnectMutex);

    mProxyCallCallback = callback;
    if (mConnection) {
        mConnection->setProxyCallCallback(callback);
    }
}

void Zone::setDbusStateChangedCallback(const DbusStateChangedCallback& callback)
{
    mDbusStateChangedCallback = callback;
}

void Zone::proxyCallAsync(const std::string& busName,
                               const std::string& objectPath,
                               const std::string& interface,
                               const std::string& method,
                               GVariant* parameters,
                               const dbus::DbusConnection::AsyncMethodCallCallback& callback)
{
    Lock lock(mReconnectMutex);
    if (mConnection) {
        mConnection->proxyCallAsync(busName,
                                    objectPath,
                                    interface,
                                    method,
                                    parameters,
                                    callback);
    } else {
        LOGE(getId() << ": Can't do a proxy call, no connection to DBUS");
    }
}

std::string Zone::declareFile(const int32_t& type,
                              const std::string& path,
                              const int32_t& flags,
                              const int32_t& mode)
{
    return mProvision->declareFile(type, path, flags, mode);
}

std::string Zone::declareMount(const std::string& source,
                               const std::string& target,
                               const std::string& type,
                               const int64_t& flags,
                               const std::string& data)
{
    return mProvision->declareMount(source, target, type, flags, data);
}

std::string Zone::declareLink(const std::string& source,
                              const std::string& target)
{
    return mProvision->declareLink(source, target);
}

std::vector<std::string> Zone::getDeclarations() const
{
    return mProvision->list();
}

void Zone::removeDeclaration(const std::string& declarationId)
{
    mProvision->remove(declarationId);
}

void Zone::setNetdevAttrs(const std::string& netdev, const ZoneAdmin::NetdevAttrs& attrs)
{
    Lock lock(mReconnectMutex);
    mAdmin->setNetdevAttrs(netdev, attrs);
}

ZoneAdmin::NetdevAttrs Zone::getNetdevAttrs(const std::string& netdev)
{
    Lock lock(mReconnectMutex);
    return mAdmin->getNetdevAttrs(netdev);
}

std::vector<std::string> Zone::getNetdevList()
{
    Lock lock(mReconnectMutex);
    return mAdmin->getNetdevList();
}

void Zone::deleteNetdevIpAddress(const std::string& netdev, const std::string& ip)
{
    Lock lock(mReconnectMutex);
    mAdmin->deleteNetdevIpAddress(netdev, ip);
}

} // namespace vasum
