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

#include "config.hpp"

#include "containers-manager.hpp"
#include "container-admin.hpp"
#include "exception.hpp"

#include "utils/paths.hpp"
#include "log/logger.hpp"
#include "config/manager.hpp"

#include <cassert>
#include <string>
#include <climits>


namespace security_containers {


ContainersManager::ContainersManager(const std::string& managerConfigPath): mDetachOnExit(false)
{
    LOGD("Instantiating ContainersManager object...");
    config::loadFromFile(managerConfigPath, mConfig);

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
        using namespace std::placeholders;
        c->setNotifyActiveContainerCallback(bind(&ContainersManager::notifyActiveContainerHandler,
                                                 this,
                                                 id,
                                                 _1,
                                                 _2));

        c->setDisplayOffCallback(bind(&ContainersManager::displayOffHandler,
                                      this,
                                      id));

        mContainers.insert(ContainerMap::value_type(id, std::move(c)));
    }

    // check if default container exists, throw ContainerOperationException if not found
    if (mContainers.find(mConfig.defaultId) == mContainers.end()) {
        LOGE("Provided default container ID " << mConfig.defaultId << " is invalid.");
        throw ContainerOperationException("Provided default container ID " + mConfig.defaultId +
                                          " is invalid.");
    }

    LOGD("ContainersManager object instantiated");

    if (mConfig.inputConfig.enabled) {
        LOGI("Registering input monitor [" << mConfig.inputConfig.device.c_str() << "]");
        mSwitchingSequenceMonitor.reset(
                new InputMonitor(mConfig.inputConfig,
                                 std::bind(&ContainersManager::switchingSequenceMonitorNotify,
                                           this)));
    }
}


ContainersManager::~ContainersManager()
{
    LOGD("Destroying ContainersManager object...");

    if (!mDetachOnExit) {
        try {
            stopAll();
        } catch (ServerException&) {
            LOGE("Failed to stop all of the containers");
        }
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

void ContainersManager::switchingSequenceMonitorNotify()
{
    LOGI("switchingSequenceMonitorNotify() called");
    // TODO: implement
}


void ContainersManager::setContainersDetachOnExit()
{
    mDetachOnExit = true;

    for (auto& container : mContainers) {
        container.second->setDetachOnExit();
    }
}

void ContainersManager::notifyActiveContainerHandler(const std::string& caller,
                                                     const std::string& application,
                                                     const std::string& message)
{
    LOGI("notifyActiveContainerHandler(" << caller << ", " << application << ", " << message
         << ") called");
    try {
        const std::string activeContainer = getRunningForegroundContainerId();
        if (!activeContainer.empty() && caller != activeContainer) {
            mContainers[activeContainer]->sendNotification(caller, application, message);
        }
    } catch(const SecurityContainersException&) {
        LOGE("Notification from " << caller << " hasn't been sent");
    }
}

void ContainersManager::displayOffHandler(const std::string& /*caller*/)
{
    LOGI("Switching to default container " << mConfig.defaultId);
    focus(mConfig.defaultId);
}

} // namespace security_containers
