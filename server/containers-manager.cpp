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
 * @brief   Definition of the class for managing containers
 */

#include "containers-manager.hpp"
#include "container-admin.hpp"
#include "exception.hpp"
#include "utils/paths.hpp"
#include "log/logger.hpp"

#include <cassert>
#include <string>
#include <climits>


namespace security_containers {


ContainersManager::ContainersManager(const std::string& managerConfigPath)
{
    LOGD("Instantiating ContainersManager object...");
    mConfig.parseFile(managerConfigPath);

    for (auto& containerConfig : mConfig.containerConfigs) {
        std::string containerConfigPath;

        if (containerConfig[0] == '/') {
            containerConfigPath = containerConfig;
        } else {
            std::string baseConfigPath = utils::dirName(managerConfigPath);
            containerConfigPath = utils::createFilePath(baseConfigPath, "/", containerConfig);
        }

        LOGD("Creating Container " << containerConfigPath);
        std::unique_ptr<Container> c(new Container(containerConfigPath));
        std::string id = c->getId();
        mContainers.emplace(id, std::move(c));
    }

    LOGD("ContainersManager object instantiated");
}


ContainersManager::~ContainersManager()
{
    LOGD("Destroying ContainersManager object...");
    try {
        stopAll();
    } catch (ServerException&) {
        LOGE("Failed to stop all of the containers");
    }
    LOGD("ContainersManager object destroyed");
}


void ContainersManager::focus(const std::string& containerId)
{
    /* try to access the object first to throw immediately if it doesn't exist */
    ContainerMap::mapped_type& foregroundContainer = mContainers.at(containerId);

    for (auto& container : mContainers) {
        LOGD(container.second->getId() << ": being sent to background");
        container.second->goBackground();
    }
    mConfig.foregroundId = foregroundContainer->getId();
    LOGD(mConfig.foregroundId << ": being sent to foreground");
    foregroundContainer->goForeground();
}


void ContainersManager::startAll()
{
    LOGI("Starting all containers");

    bool isForegroundFound = false;

    for (auto& container : mContainers) {
        container.second->start();

        if (container.first == mConfig.foregroundId) {
            isForegroundFound = true;
            LOGI(container.second->getId() << ": set as the foreground container");
            container.second->goForeground();
        }
    }

    if (!isForegroundFound) {
        auto foregroundIterator = std::min_element(mContainers.begin(), mContainers.end(),
                                                   [](ContainerMap::value_type &c1, ContainerMap::value_type &c2) {
                                                       return c1.second->getPrivilege() < c2.second->getPrivilege();
                                                   });

        mConfig.foregroundId = foregroundIterator->second->getId();
        LOGI(mConfig.foregroundId << ": no foreground container configured, setting one with highest priority");
        foregroundIterator->second->goForeground();
    }
}


void ContainersManager::stopAll()
{
    LOGI("Stopping all containers");

    for (auto& container : mContainers) {
        container.second->stop();
    }
}


std::string ContainersManager::getRunningForegroundContainerId()
{
    for (auto& container : mContainers) {
        if (container.first == mConfig.foregroundId &&
            container.second->isRunning()) {
            return container.first;
        }
    }
    return std::string();
}


} // namespace security_containers
