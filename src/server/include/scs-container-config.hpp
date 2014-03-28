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
 * @file    scs-container-config.hpp
 * @author  Michal Witanowski (m.witanowski@samsung.com)
 * @brief   Declaration of the class for storing container configuration
 */


#ifndef SECURITY_CONTAINERS_SERVER_CONTAINER_CONFIG_HPP
#define SECURITY_CONTAINERS_SERVER_CONTAINER_CONFIG_HPP

#include "scs-configuration.hpp"
#include <string>

namespace security_containers {

struct ContainerConfig : public ConfigurationBase {
    /**
     * Privilege of the container.
     * The smaller the value the more important the container
     */
    int privilege;

    /**
     * Container's libvirt (XML) config file.
     * Location can be relative to the Container's config file.
     */
    std::string config;

    /**
     * Container's CFS quota in us when it's in the foreground
     */
    double cpuQuotaForeground;

    /**
     * Container's CFS quota in us when it's in the background
     */
    double cpuQuotaBackground;

    CONFIG_REGISTER {
        CONFIG_VALUE(privilege)
        CONFIG_VALUE(config)
        CONFIG_VALUE(cpuQuotaForeground)
        CONFIG_VALUE(cpuQuotaBackground)
    }
};

}

#endif // SECURITY_CONTAINERS_SERVER_CONTAINER_CONFIG_HPP
