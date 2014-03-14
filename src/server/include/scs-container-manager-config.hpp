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
 * @file    scs-container-manager-config.hpp
 * @author  Jan Olszak (j.olszak@samsung.com)
 * @brief   Declaration of the class for storing container manager configuration
 */


#ifndef SECURITY_CONTAINERS_SERVER_CONTAINER_MANAGER_CONFIG_HPP
#define SECURITY_CONTAINERS_SERVER_CONTAINER_MANAGER_CONFIG_HPP

#include "scs-configuration.hpp"
#include <string>
#include <vector>

namespace security_containers {

const std::string CONTAINER_MANAGER_CONFIG_PATH = "/etc/security-containers/config/.daemon.json";

struct ContainerManagerConfig : public ConfigurationBase {
    /**
     * Directory with the libvirt's configuration files
     */
    std::string libvirtConfigDir;

    /**
     * List of containers that we manage.
     * Appropriate files have to be inside config file
     */
    std::vector<std::string> containerIds;

    CONFIG_REGISTER {
        CONFIG_VALUE(libvirtConfigDir)
        CONFIG_VALUE(containerIds)
    }
};

}

#endif // SECURITY_CONTAINERS_SERVER_CONTAINER_MANAGER_CONFIG_HPP