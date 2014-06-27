/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Michal Witanowski <m.witanowski@samsung.com>
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
 * @author  Michal Witanowski (m.witanowski@samsung.com)
 * @brief   Declaration of the class for storing container configuration
 */


#ifndef SERVER_CONTAINER_CONFIG_HPP
#define SERVER_CONTAINER_CONFIG_HPP

#include "config/fields.hpp"

#include <string>
#include <vector>


namespace security_containers {


struct ContainerConfig {
    /**
     * Privilege of the container.
     * The smaller the value the more important the container
     */
    int privilege;

    /**
     * Allow switching to default container after timeout.
     * Setting this to false will disable switching to default container after timeout.
     */
    bool switchToDefaultAfterTimeout;

    /**
     * Container's libvirt (XML) config file.
     * Location can be relative to the Container's config file.
     */
    std::string config;

    /**
     * Container's libvirt (XML) network config file.
     */
    std::string networkConfig;

    /**
     *
     * Container's libvirt (XML) network filter config file.
     */
    std::string networkFilterConfig;

    /**
     * Container's CFS quota in us when it's in the foreground
     */
    std::int64_t cpuQuotaForeground;

    /**
     * Container's CFS quota in us when it's in the background
     */
    std::int64_t cpuQuotaBackground;

    /**
     * Path to containers dbus unix socket
     */
    std::string runMountPoint;

    /**
     * When you move a file out of the container (by move request)
     * its path must match at least one of the regexps in this vector.
     */
    std::vector<std::string> permittedToSend;

    /**
     * When you move a file to the container (by move request)
     * its path must match at least one of the regexps in this vector.
     */
    std::vector<std::string> permittedToRecv;

    CONFIG_REGISTER
    (
        privilege,
        switchToDefaultAfterTimeout,
        config,
        networkConfig,
        networkFilterConfig,
        cpuQuotaForeground,
        cpuQuotaBackground,
        runMountPoint,
        permittedToSend,
        permittedToRecv
    )
};


}


#endif // SERVER_CONTAINER_CONFIG_HPP
