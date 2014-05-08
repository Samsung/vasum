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
 * @brief   Declaration of the class for managing one container
 */


#ifndef SERVER_CONTAINER_HPP
#define SERVER_CONTAINER_HPP

#include "container-config.hpp"
#include "container-admin.hpp"
#include "container-connection.hpp"
#include "container-connection-transport.hpp"

#include <string>
#include <memory>
#include <thread>


namespace security_containers {


class Container {

public:
    Container(const std::string& containerConfigPath);
    Container(Container&&) = default;
    virtual ~Container();

    /**
     * Get the container id
     */
    const std::string& getId() const;

    /**
     * Get the container privilege
     */
    int getPrivilege() const;

    /**
     * Boot the container to the background.
     */
    void start();

    /**
     * Forcefully stop the container.
     */
    void stop();

    /**
     * Setup this container to be put in the foreground.
     * I.e. set appropriate scheduler level.
     */
    void goForeground();

    /**
     * Setup this container to be put in the background.
     * I.e. set appropriate scheduler level.
     */
    void goBackground();

    /**
     * @return Is the container running?
     */
    bool isRunning();

    /**
     * Check if the container is stopped. It's NOT equivalent to !isRunning,
     * because it checks different internal libvirt's states. There are other states,
     * (e.g. paused) when the container isn't running nor stopped.
     *
     * @return Is the container stopped?
     */
    bool isStopped();

    /**
     * @return Is the container in a paused state?
     */
    bool isPaused();

private:
    ContainerConfig mConfig;
    std::unique_ptr<ContainerConnectionTransport> mConnectionTransport;
    std::unique_ptr<ContainerAdmin> mAdmin;
    std::unique_ptr<ContainerConnection> mConnection;
    std::thread mReconnectThread;

    void onNameLostCallback();
    void reconnectHandler();
};


}


#endif // SERVER_CONTAINER_HPP
