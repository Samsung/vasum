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
     * Lxc template name (relative to lxcTemplatePrefix)
     */
    std::string lxcTemplate;

    /**
     * Init program with args (empty means default /sbin/init)
     */
    std::vector<std::string> initWithArgs;

    /**
     * Privilege of the zone.
     * The smaller the value the more important the zone
     */
    int privilege;

    /**
     * Allow switching to default zone after timeout.
     * Setting this to false will disable switching to default zone after timeout.
     */
    bool switchToDefaultAfterTimeout;

    /**
      * Specify, if D-Bus communication with the zone will be enabled.
      * Setting this value to "false" will make the zone API not work inside the zone.
      */
    bool enableZoneConnection;

    /**
     * Zone's CFS quota in us when it's in the foreground
     */
    std::int64_t cpuQuotaForeground;

    /**
     * Zone's CFS quota in us when it's in the background
     */
    std::int64_t cpuQuotaBackground;

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

    /**
     * Timeout in seconds for zone to gracefully shut down.
     * After given time, if Zone is not off, forced shutdown occurs.
     *
     * To wait forever, set -1 timeout. To skip waiting, set 0 timeout.
     */
    int shutdownTimeout;

    CONFIG_REGISTER
    (
        lxcTemplate,
        initWithArgs,
        privilege, // TODO not needed?
        switchToDefaultAfterTimeout, // TODO move to dynamic and add an API to change
        enableZoneConnection,
        cpuQuotaForeground,
        cpuQuotaBackground,
        permittedToSend, // TODO move to dynamic and add an API to change
        permittedToRecv, // TODO move to dynamic and add an API to change
        validLinkPrefixes,
        shutdownTimeout
    )
};

struct ZoneDynamicConfig {

    /**
     * Requested zone state after restore
     */
    std::string requestedState;

    /**
     * IP v4 gateway address
     */
    std::string ipv4Gateway;

    /**
     * IP v4 address
     */
    std::string ipv4;

    /**
     * Number of virtual terminal used by xserver inside zone
     */
    int vt;

    /**
     * Path to zones dbus unix socket
     */
    std::string runMountPoint;

    CONFIG_REGISTER
    (
        requestedState,
        ipv4Gateway,
        ipv4,
        vt,
        runMountPoint
    )
};

struct ZoneTemplatePathConfig {
    /**
     * A path to zone template config (containing default values)
     */
    std::string zoneTemplatePath;

    CONFIG_REGISTER(zoneTemplatePath)
};

} // namespace vasum

#endif // SERVER_ZONE_CONFIG_HPP
