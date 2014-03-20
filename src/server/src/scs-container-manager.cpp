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
 * @file    scs-container-manager.cpp
 * @author  Jan Olszak (j.olszak@samsung.com)
 * @brief   Definition of the class for managing containers
 */

#include "scs-container-manager.hpp"
#include "scs-container-admin.hpp"
#include "scs-exception.hpp"
#include "scs-utils.hpp"
#include "scs-log.hpp"

#include <assert.h>
#include <string>

namespace security_containers {

ContainerManager::ContainerManager(const std::string& managerConfigPath)
{
    mConfig.parseFile(managerConfigPath);
    connect();

    for (auto& containerConfig : mConfig.containerConfigs) {
        std::string containerConfigPath;

        if (containerConfig[0] == '/') {
            containerConfigPath = containerConfig;
        } else {
            std::string baseConfigPath = dirName(managerConfigPath);
            containerConfigPath = createFilePath(baseConfigPath, "/", containerConfig);
        }

        LOGT("Creating Container " << containerConfigPath);
        std::unique_ptr<Container> c(new Container(containerConfigPath));
        std::string id = c->getAdmin().getId();
        mContainers.emplace(id, std::move(c));
    }
}


ContainerManager::~ContainerManager()
{
    try {
        stopAll();
    } catch (ServerException& e) {
        LOGE("Failed to stop all of the containers");
    }
    disconnect();
}


void ContainerManager::focus(const std::string& containerId)
{
    for (auto& container : mContainers) {
        container.second->getAdmin().suspend();
    }
    mContainers.at(containerId)->getAdmin().resume();
}


void ContainerManager::startAll()
{
    for (auto& container : mContainers) {
        container.second->getAdmin().start();
    }
}


void ContainerManager::stopAll()
{
    for (auto& container : mContainers) {
        container.second->getAdmin().stop();
    }
}


std::string ContainerManager::getRunningContainerId()
{
    for (auto& container : mContainers) {
        if (container.second->getAdmin().isRunning()) {
            return container.first;
        }
    }
    return "";
}


std::vector<std::string> ContainerManager::getSuspendedContainerIds()
{
    std::vector<std::string> retContainerIds;
    for (auto& container : mContainers) {
        if (container.second->getAdmin().isPaused()) {
            retContainerIds.push_back(container.first);
        }
    }
    return retContainerIds;
}


void ContainerManager::connect()
{
    assert(mVir == NULL);

    mVir = virConnectOpen("lxc://");
    if (mVir == NULL) {
        LOGE("Failed to open connection to lxc://");
        throw ConnectionException();
    }
}


void ContainerManager::disconnect()
{
    if (mVir == NULL) {
        return;
    }

    if (virConnectClose(mVir) < 0) {
        LOGE("Error during unconnecting from libvirt");
    };
    mVir = NULL;
}


} // namespace security_containers
