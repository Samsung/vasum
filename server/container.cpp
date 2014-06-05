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

#include <string>
#include <thread>


namespace security_containers {


namespace {

// TODO: move constants to the config file when default values are implemented there
const int RECONNECT_RETRIES = 15;
const int RECONNECT_DELAY = 1 * 1000;

} // namespace

Container::Container(const std::string& containerConfigPath)
{
    config::loadFromFile(containerConfigPath, mConfig);
    std::string libvirtConfigPath;

    if (mConfig.config[0] == '/') {
        libvirtConfigPath = mConfig.config;
    } else {
        std::string baseConfigPath = utils::dirName(containerConfigPath);
        libvirtConfigPath = utils::createFilePath(baseConfigPath, "/", mConfig.config);
    }

    mConfig.config = libvirtConfigPath;
    LOGT("Creating Container Admin " << mConfig.config);
    mAdmin.reset(new ContainerAdmin(mConfig));
}

Container::~Container()
{
    // Make sure all OnNameLostCallbacks get finished and no new will
    // get called before proceeding further. This guarantees no race
    // condition on the mReconnectThread.
    mConnection.reset();

    if (mReconnectThread.joinable()) {
        mReconnectThread.join();
    }
}

const std::string& Container::getId() const
{
    return mAdmin->getId();
}

int Container::getPrivilege() const
{
    return mConfig.privilege;
}

void Container::start()
{
    mConnectionTransport.reset(new ContainerConnectionTransport(mConfig.runMountPoint));
    mAdmin->start();
    mConnection.reset(new ContainerConnection(mConnectionTransport->acquireAddress(),
                                              std::bind(&Container::onNameLostCallback, this)));

    // Send to the background only after we're connected,
    // otherwise it'd take ages.
    LOGD(getId() << ": DBUS connected, sending to the background");
    goBackground();
}

void Container::stop()
{
    mConnection.reset();
    mAdmin->stop();
    mConnectionTransport.reset();
}

void Container::goForeground()
{
    mAdmin->setSchedulerLevel(SchedulerLevel::FOREGROUND);
}

void Container::goBackground()
{
    mAdmin->setSchedulerLevel(SchedulerLevel::BACKGROUND);
}

void Container::setDetachOnExit()
{
    mAdmin->setDetachOnExit();
    mConnectionTransport->setDetachOnExit();
}

bool Container::isRunning()
{
    return mAdmin->isRunning();
}

bool Container::isStopped()
{
    return mAdmin->isStopped();
}

bool Container::isPaused()
{
    return mAdmin->isPaused();
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
    mConnection.reset();

    for (int i = 0; i < RECONNECT_RETRIES; ++i) {
        // This sleeps even before the first try to give DBUS some time to come back up
        std::this_thread::sleep_for(std::chrono::milliseconds(RECONNECT_DELAY));

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


} // namespace security_containers
