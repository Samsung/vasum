/*
 *  Copyright (c) 2000 - 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Bumjin Im <bj.im@samsung.com>
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
}

std::string Container::getId()
{
    return mAdmin->getId();
}

int Container::getPrivilege()
{
    return mConfig.privilege;
}

void Container::start()
{
    return mAdmin->start();
}

void Container::stop()
{
    return mAdmin->stop();
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


} // namespace security_containers
