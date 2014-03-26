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
 * @file    scs-container.cpp
 * @author  Lukasz Pawelczyk (l.pawelczyk@partner.samsung.com)
 * @brief   Implementation of class for managing one container
 */

#include "scs-container.hpp"
#include "utils.hpp"
#include "log.hpp"

#include <assert.h>
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

    LOGT("Creating Container Admin " << libvirtConfigPath);
    mAdmin.reset(new ContainerAdmin(libvirtConfigPath));
}


Container::~Container()
{
}

ContainerAdmin& Container::getAdmin()
{
    assert(mAdmin.get() != NULL);

    return *mAdmin;
}

} // namespace security_containers
