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
 * @brief   Declaration of the class for storing zone manager configuration
 */


#ifndef SERVER_ZONES_MANAGER_CONFIG_HPP
#define SERVER_ZONES_MANAGER_CONFIG_HPP

#include "config/fields.hpp"
#include "input-monitor-config.hpp"
#include "proxy-call-config.hpp"

#include <string>
#include <vector>


namespace vasum {

struct ZonesManagerConfig {

    /**
     * Path to config database.
     */
    std::string dbPath;

    /**
     * A path where the zones mount points reside.
     */
    std::string zonesPath;

    /**
     * A path where the zones image reside. Empty path means that zones image won't be
     * copied to zonesPath when creating new zone.
     */
    std::string zoneImagePath;

    /**
     * A path where template configuration files for new zones reside
     */
    std::string zoneTemplatePath;

    /**
     * Prefix added to a path for new zone configuration files
     */
    std::string zoneNewConfigPrefix;

    /**
     * Path prefix for lxc templates
     */
    std::string lxcTemplatePrefix;

    /*
     * Parameters describing input device used to switch between zones
     */
    InputConfig inputConfig;

    /**
     * Prefix added to a path of "run" tmpfs mount point for each zone.
     */
    std::string runMountPointPrefix;

    /**
     * Proxy call rules.
     */
    std::vector<ProxyCallRule> proxyCallRules;

    CONFIG_REGISTER
    (
        dbPath,
        zonesPath,
        zoneImagePath,
        zoneTemplatePath,
        zoneNewConfigPrefix,
        lxcTemplatePrefix,
        inputConfig,
        runMountPointPrefix,
        proxyCallRules
    )
};

struct ZonesManagerDynamicConfig {

    /**
     * List of zones' configs that we manage.
     * File paths can be relative to the ZoneManager config file.
     */
    std::vector<std::string> zoneConfigs;

    /**
     * An ID of default zone.
     */
    std::string defaultId;

    CONFIG_REGISTER
    (
        zoneConfigs,
        defaultId
    )
};

} // namespace vasum


#endif // SERVER_ZONES_MANAGER_CONFIG_HPP
