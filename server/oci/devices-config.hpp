/*
 *  Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Dariusz Michaluk <d.michaluk@samsung.com>
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
 * @author  Dariusz Michaluk (d.michaluk@samsung.com)
 * @brief   Devices configuration
 */

#ifndef DEVICES_CONFIG_HPP
#define DEVICES_CONFIG_HPP

#include "config.hpp"
#include "cargo/fields.hpp"

#include <string>
#include <vector>

namespace vasum {

struct DeviceConfig {

    /**
     * Full path to the device inside container.
     */
    std::string path;

    /**
     * Device type, block, char, etc.
     */
    char type;

    /**
     * Device's major, minor number.
     */
    int64_t major;
    int64_t minor;

    /**
     * Cgroup permissions format, rwm.
     */
    std::string permissions;

    /**
     * File mode permission bits for the device.
     */
    uint32_t fileMode;

    /**
     * UID, GID of the device owner.
     */
    uint32_t uid;
    uint32_t gid;

    CARGO_REGISTER
    (
        path,
        type,
        major,
        minor,
        permissions,
        fileMode,
        uid,
        gid
    )
};

typedef std::vector<DeviceConfig> DevicesConfig;

} // namespace vasum


#endif // DEVICES_CONFIG_HPP
