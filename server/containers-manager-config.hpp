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
 * @brief   Declaration of the class for storing container manager configuration
 */


#ifndef SERVER_CONTAINERS_MANAGER_CONFIG_HPP
#define SERVER_CONTAINERS_MANAGER_CONFIG_HPP

#include "config/fields.hpp"
#include "input-monitor-config.hpp"
#include "proxy-call-config.hpp"

#include <string>
#include <vector>


namespace security_containers {


const std::string CONTAINERS_MANAGER_CONFIG_PATH = "/etc/security-containers/config/daemon.conf";

struct ContainersManagerConfig {

    /**
     * List of containers' configs that we manage.
     * File paths can be relative to the ContainerManager config file.
     */
    std::vector<std::string> containerConfigs;

    /**
     * An ID of a currently focused/foreground container.
     */
    std::string foregroundId;

    /**
     * An ID of default container.
     */
    std::string defaultId;

    /**
     * A path where the containers mount points reside.
     */
    std::string containersPath;

    /**
     * A path where the containers image reside. Empty path means that containers image won't be
     * copied to containersPath when creating new container.
     */
    std::string containerImagePath;

    /**
     * A path where template configuration files for new containers reside
     */
    std::string containerTemplatePath;

    /**
     * Prefix added to a path for new container configuration files
     */
    std::string containerNewConfigPrefix;

    /**
     * Path prefix for lxc templates
     */
    std::string lxcTemplatePrefix;

    /*
     * Parameters describing input device used to switch between containers
     */
    InputConfig inputConfig;

    /**
     * Prefix added to a path of "run" tmpfs mount point for each container.
     */
    std::string runMountPointPrefix;

    /**
     * Proxy call rules.
     */
    std::vector<ProxyCallRule> proxyCallRules;

    CONFIG_REGISTER
    (
        containerConfigs,
        foregroundId,
        defaultId,
        containersPath,
        containerImagePath,
        containerTemplatePath,
        containerNewConfigPrefix,
        lxcTemplatePrefix,
        inputConfig,
        runMountPointPrefix,
        proxyCallRules
    )
};


} // namespace security_containers


#endif // SERVER_CONTAINERS_MANAGER_CONFIG_HPP
