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
 * @brief   Declaration of the class for managing many containers
 */


#ifndef SERVER_CONTAINERS_MANAGER_HPP
#define SERVER_CONTAINERS_MANAGER_HPP

#include "container.hpp"
#include "containers-manager-config.hpp"
#include "host-connection.hpp"
#include "input-monitor.hpp"
#include "proxy-call-policy.hpp"

#include <string>
#include <unordered_map>
#include <libvirt/libvirt.h>
#include <memory>


namespace security_containers {


class ContainersManager final {

public:
    ContainersManager(const std::string& managerConfigPath);
    ~ContainersManager();

    /**
     * Add new container.
     *
     * @param containerConfig config of new container
     */
    void addContainer(const std::string& containerConfig);

    /**
     * Focus this container, put it to the foreground.
     * Method blocks until the focus is switched.
     *
     * @param containerId id of the container
     */
    void focus(const std::string& containerId);

    /**
     * Start up all the configured containers
     */
    void startAll();

    /**
     * Stop all managed containers
     */
    void stopAll();

    /**
     * @return id of the currently focused/foreground container
     */
    std::string getRunningForegroundContainerId();

    /**
     * @return id of next to currently focused/foreground container. If currently focused container
     *         is last in container map, id of fisrt container from map is returned.
     */
    std::string getNextToForegroundContainerId();

    /**
     * Set whether ContainersManager should detach containers on exit
     */
    void setContainersDetachOnExit();

private:
    ContainersManagerConfig mConfig;
    std::string mConfigPath;
    HostConnection mHostConnection;
    // to hold InputMonitor pointer to monitor if container switching sequence is recognized
    std::unique_ptr<InputMonitor> mSwitchingSequenceMonitor;
    std::unique_ptr<ProxyCallPolicy> mProxyCallPolicy;
    typedef std::unordered_map<std::string, std::unique_ptr<Container>> ContainerMap;
    ContainerMap mContainers; // map of containers, id is the key
    bool mDetachOnExit;

    void switchingSequenceMonitorNotify();
    void generateNewConfig(const std::string& id,
                           const std::string& templatePath,
                           const std::string& resultPath);

    void notifyActiveContainerHandler(const std::string& caller,
                                      const std::string& appliaction,
                                      const std::string& message);
    void displayOffHandler(const std::string& caller);
    void handleContainerMoveFileRequest(const std::string& srcContainerId,
                                        const std::string& dstContainerId,
                                        const std::string& path,
                                        dbus::MethodResultBuilder::Pointer result);
    void handleProxyCall(const std::string& caller,
                         const std::string& target,
                         const std::string& targetBusName,
                         const std::string& targetObjectPath,
                         const std::string& targetInterface,
                         const std::string& targetMethod,
                         GVariant* parameters,
                         dbus::MethodResultBuilder::Pointer result);
    void handleGetContainerDbuses(dbus::MethodResultBuilder::Pointer result);
    void handleDbusStateChanged(const std::string& containerId, const std::string& dbusAddress);
    void handleGetContainerIdsCall(dbus::MethodResultBuilder::Pointer result);
    void handleGetActiveContainerIdCall(dbus::MethodResultBuilder::Pointer result);
    void handleGetContainerInfoCall(const std::string& id, dbus::MethodResultBuilder::Pointer result);
    void handleSetActiveContainerCall(const std::string& id,
                                      dbus::MethodResultBuilder::Pointer result);
    void handleAddContainerCall(const std::string& id,
                                          dbus::MethodResultBuilder::Pointer result);
};


} // namespace security_containers


#endif // SERVER_CONTAINERS_MANAGER_HPP
