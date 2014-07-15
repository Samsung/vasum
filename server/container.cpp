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
 * @brief   Implementation of class for managing one container
 */

#include "config.hpp"

#include "container.hpp"

#include "log/logger.hpp"
#include "utils/paths.hpp"
#include "config/manager.hpp"

#include <boost/filesystem.hpp>

#include <string>
#include <thread>


namespace security_containers {

namespace fs = boost::filesystem;

namespace {

typedef std::lock_guard<std::recursive_mutex> Lock;

// TODO: move constants to the config file when default values are implemented there
const int RECONNECT_RETRIES = 15;
const int RECONNECT_DELAY = 1 * 1000;

} // namespace

Container::Container(const std::string& containerConfigPath,
                     const std::string& baseRunMountPointPath)
{
    config::loadFromFile(containerConfigPath, mConfig);

    for (std::string r: mConfig.permittedToSend) {
        mPermittedToSend.push_back(boost::regex(r));
    }
    for (std::string r: mConfig.permittedToRecv) {
        mPermittedToRecv.push_back(boost::regex(r));
    }

    const std::string baseConfigPath = utils::dirName(containerConfigPath);
    mConfig.config = fs::absolute(mConfig.config, baseConfigPath).string();
    mConfig.networkConfig = fs::absolute(mConfig.networkConfig, baseConfigPath).string();
    if (!mConfig.runMountPoint.empty()) {
        mRunMountPoint = fs::absolute(mConfig.runMountPoint, baseRunMountPointPath).string();
    }

    LOGT("Creating Network Admin " << mConfig.networkConfig);
    mNetworkAdmin.reset(new NetworkAdmin(mConfig));
    LOGT("Creating Container Admin " << mConfig.config);
    mAdmin.reset(new ContainerAdmin(mConfig));
}

Container::~Container()
{
    // Make sure all OnNameLostCallbacks get finished and no new will
    // get called before proceeding further. This guarantees no race
    // condition on the mReconnectThread.
    {
        Lock lock(mReconnectMutex);
        mConnection.reset();
    }

    if (mReconnectThread.joinable()) {
        mReconnectThread.join();
    }
}

const std::vector<boost::regex>& Container::getPermittedToSend() const
{
    return mPermittedToSend;
}

const std::vector<boost::regex>& Container::getPermittedToRecv() const
{
    return mPermittedToRecv;
}

const std::string& Container::getId() const
{
    Lock lock(mReconnectMutex);
    return mAdmin->getId();
}

int Container::getPrivilege() const
{
    return mConfig.privilege;
}

void Container::start()
{
    Lock lock(mReconnectMutex);
    mConnectionTransport.reset(new ContainerConnectionTransport(mRunMountPoint));
    mNetworkAdmin->start();
    mAdmin->start();
    mConnection.reset(new ContainerConnection(mConnectionTransport->acquireAddress(),
                                              std::bind(&Container::onNameLostCallback, this)));
    if (mNotifyCallback) {
        mConnection->setNotifyActiveContainerCallback(mNotifyCallback);
    }
    if (mDisplayOffCallback) {
        mConnection->setDisplayOffCallback(mDisplayOffCallback);
    }
    if (mFileMoveCallback) {
        mConnection->setFileMoveRequestCallback(mFileMoveCallback);
    }
    if (mProxyCallCallback) {
        mConnection->setProxyCallCallback(mProxyCallCallback);
    }

    // Send to the background only after we're connected,
    // otherwise it'd take ages.
    LOGD(getId() << ": DBUS connected, sending to the background");
    goBackground();
}

void Container::stop()
{
    Lock lock(mReconnectMutex);
    mConnection.reset();
    mAdmin->stop();
    mNetworkAdmin->stop();
    mConnectionTransport.reset();
}

void Container::goForeground()
{
    Lock lock(mReconnectMutex);
    mAdmin->setSchedulerLevel(SchedulerLevel::FOREGROUND);
}

void Container::goBackground()
{
    Lock lock(mReconnectMutex);
    mAdmin->setSchedulerLevel(SchedulerLevel::BACKGROUND);
}

void Container::setDetachOnExit()
{
    Lock lock(mReconnectMutex);
    mNetworkAdmin->setDetachOnExit();
    mAdmin->setDetachOnExit();
    mConnectionTransport->setDetachOnExit();
}

bool Container::isRunning()
{
    Lock lock(mReconnectMutex);
    return mAdmin->isRunning();
}

bool Container::isStopped()
{
    Lock lock(mReconnectMutex);
    return mAdmin->isStopped();
}

bool Container::isPaused()
{
    Lock lock(mReconnectMutex);
    return mAdmin->isPaused();
}

bool Container::isSwitchToDefaultAfterTimeoutAllowed() const
{
    return mConfig.switchToDefaultAfterTimeout;
}

void Container::onNameLostCallback()
{
    LOGI(getId() << ": A connection to the DBUS server has been lost, reconnecting...");

    if (mReconnectThread.joinable()) {
        mReconnectThread.join();
    }
    mReconnectThread = std::thread(std::bind(&Container::reconnectHandler, this));
}

void Container::reconnectHandler()
{
    {
        Lock lock(mReconnectMutex);
        mConnection.reset();
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
            mConnection.reset(new ContainerConnection(mConnectionTransport->acquireAddress(),
                                                      std::bind(&Container::onNameLostCallback, this)));
            LOGI(getId() << ": Reconnected");
            return;
        } catch (SecurityContainersException&) {
            LOGT(getId() << ": Reconnect try " << i + 1 << " has been unsuccessful");
        }
    }

    LOGE(getId() << ": Reconnecting to the DBUS has failed, stopping the container");
    stop();
}

void Container::setNotifyActiveContainerCallback(const NotifyActiveContainerCallback& callback)
{
    Lock lock(mReconnectMutex);
    mNotifyCallback = callback;
    if (mConnection) {
        mConnection->setNotifyActiveContainerCallback(mNotifyCallback);
    }
}

void Container::sendNotification(const std::string& container,
                                 const std::string& application,
                                 const std::string& message)
{
    Lock lock(mReconnectMutex);
    if (mConnection) {
        mConnection->sendNotification(container, application, message);
    } else {
        LOGE(getId() << ": Can't send notification, no connection to DBUS");
    }
}

void Container::setDisplayOffCallback(const DisplayOffCallback& callback)
{
    Lock lock(mReconnectMutex);

    mDisplayOffCallback = callback;
    if (mConnection) {
        mConnection->setDisplayOffCallback(callback);
    }
}

void Container::setFileMoveRequestCallback(const FileMoveRequestCallback& callback)
{
    Lock lock(mReconnectMutex);

    mFileMoveCallback = callback;
    if (mConnection) {
        mConnection->setFileMoveRequestCallback(callback);
    }
}

void Container::setProxyCallCallback(const ProxyCallCallback& callback)
{
    Lock lock(mReconnectMutex);

    mProxyCallCallback = callback;
    if (mConnection) {
        mConnection->setProxyCallCallback(callback);
    }
}

void Container::proxyCallAsync(const std::string& busName,
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


} // namespace security_containers
