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
 * @file    scs-container.hpp
 * @author  Lukasz Pawelczyk (l.pawelczyk@partner.samsung.com)
 * @brief   Declaration of the class for managing one container
 */


#ifndef SECURITY_CONTAINERS_SERVER_CONTAINER_HPP
#define SECURITY_CONTAINERS_SERVER_CONTAINER_HPP

#include "scs-container-config.hpp"
#include "scs-container-admin.hpp"

#include <string>
#include <memory>

namespace security_containers {

class Container {

public:
    Container(const std::string& containerConfigPath);
    Container(Container&&) = default;
    virtual ~Container();

    /**
     * Get the container id
     */
    std::string getId();

    /**
     * Get the container privilege
     */
    int getPrivilege();

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
     * (e.g. paused) when the container isn't runnig nor stopped.
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
    std::unique_ptr<ContainerAdmin> mAdmin;
};

}

#endif // SECURITY_CONTAINERS_SERVER_CONTAINER_HPP
