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
 * @brief   Declaration of the class for storing zone configuration
 */


#ifndef SERVER_ZONE_CONFIG_HPP
#define SERVER_ZONE_CONFIG_HPP

#include "config/fields.hpp"

#include <string>
#include <vector>


namespace vasum {


struct ZoneConfig {

    /**
     * Zone name
     */
    std::string name;

    /**
     * Lxc template name (relative to lxcTemplatePrefix)
     */
    std::string lxcTemplate;

    /**
     * Init program with args (empty means default /sbin/init)
     */
    std::vector<std::string> initWithArgs;

    /**
     * IP v4 gateway address
     */
    std::string ipv4Gateway;

    /**
     * IP v4 address
     */
    std::string ipv4;

    /**
     * Privilege of the zone.
     * The smaller the value the more important the zone
     */
    int privilege;

    /**
     * Number of virtual terminal used by xserver inside zone
     */
    int vt;

    /**
     * Allow switching to default zone after timeout.
     * Setting this to false will disable switching to default zone after timeout.
     */
    bool switchToDefaultAfterTimeout;

    /**
      * Specify, if D-Bus communication with the zone will be enabled.
      * Setting this value to "false" will make the zone API not work inside the zone.
      */
    bool enableDbusIntegration;

    /**
     * Zone's CFS quota in us when it's in the foreground
     */
    std::int64_t cpuQuotaForeground;

    /**
     * Zone's CFS quota in us when it's in the background
     */
    std::int64_t cpuQuotaBackground;

    /**
     * Path to zones dbus unix socket
     */
    std::string runMountPoint;

    /**
     * When you move a file out of the zone (by move request)
     * its path must match at least one of the regexps in this vector.
     */
    std::vector<std::string> permittedToSend;

    /**
     * When you move a file to the zone (by move request)
     * its path must match at least one of the regexps in this vector.
     */
    std::vector<std::string> permittedToRecv;

    /**
     * Valid hard link prefixes.
     */
    std::vector<std::string> validLinkPrefixes;

    CONFIG_REGISTER
    (
        name,
        lxcTemplate,
        initWithArgs,
        ipv4Gateway,
        ipv4,
        privilege,
        vt,
        switchToDefaultAfterTimeout,
        enableDbusIntegration,
        cpuQuotaForeground,
        cpuQuotaBackground,
        runMountPoint,
        permittedToSend,
        permittedToRecv,
        validLinkPrefixes
    )
};

struct ZoneDynamicConfig {
    //TODO a place for zone dynamic config (other than provisioning which has its own struct)
    CONFIG_REGISTER_EMPTY
};

} // namespace vasum

#endif // SERVER_ZONE_CONFIG_HPP
