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
#include "scs-log.hpp"
#include "scs-utils.hpp"

#include <assert.h>
#include <string>
#include <fstream>
#include <streambuf>

namespace security_containers {

ContainerManager::ContainerManager(const std::string& configFilePath)
{
    mConfig.parseFile(configFilePath);
    connect();
    for (auto& containerId : mConfig.containerIds) {
        std::string libvirtConfigPath = createFilePath(mConfig.libvirtConfigDir, containerId, ".xml");
        std::ifstream t(libvirtConfigPath);
        std::string libvirtConfig((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
        LOGT("Creating Container " << libvirtConfigPath);
        mContainers.emplace(containerId, libvirtConfig);
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
        container.second.suspend();
    }
    mContainers.at(containerId).resume();
}


void ContainerManager::startAll()
{
    for (auto& container : mContainers) {
        container.second.start();
    }
}


void ContainerManager::stopAll()
{
    for (auto& container : mContainers) {
        container.second.stop();
    }
}


std::string ContainerManager::getRunningContainerId()
{
    for (auto& container : mContainers) {
        if (container.second.isRunning()) {
            return container.first;
        }
    }
    return "";
}


std::vector<std::string> ContainerManager::getSuspendedContainerIds()
{
    std::vector<std::string> retContainerIds;
    for (auto& container : mContainers) {
        if (container.second.isPaused()) {
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
