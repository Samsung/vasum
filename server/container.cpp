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

#include "container.hpp"

#include "log/logger.hpp"
#include "utils/paths.hpp"

#include <string>
#include <thread>


namespace security_containers {


Container::Container(const std::string& containerConfigPath)
{
    mConfig.parseFile(containerConfigPath);
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
    LOGD(getId() << ": Sending to the background");
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
    std::string address;

    mConnection.reset();

    try {
        address = mConnectionTransport->acquireAddress();
    } catch (SecurityContainersException&) {
        LOGE(getId() << "The socket does not exist anymore, something went terribly wrong, stopping the container");
        stop(); // TODO: shutdownOrStop()
        return;
    }

    try {
        mConnection.reset(new ContainerConnection(address, std::bind(&Container::onNameLostCallback, this)));
        LOGI(getId() << ": Reconnected");
    } catch (SecurityContainersException&) {
        LOGE(getId() << ": Reconnecting to the DBUS has failed, stopping the container");
        stop(); // TODO: shutdownOrStop()
        return;
    }
}


} // namespace security_containers
