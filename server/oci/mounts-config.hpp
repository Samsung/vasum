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
 * @brief   Mounts configuration
 */

#ifndef MOUNTS_CONFIG_HPP
#define MOUNTS_CONFIG_HPP

#include "cargo/fields.hpp"

#include <map>
#include <vector>

namespace vasum {

struct MountConfig {

    /**
     * Name of mount point. Used for config lookup.
     */
    std::string name;

    /**
     * Destination of mount point: path inside container.
     */
    std::string path;

    CARGO_REGISTER
    (
        name,
        path
    )
};

typedef std::vector<MountConfig> MountsConfig;

struct MountRuntimeConfig {

    /**
     * Filesystem type argument supported by the kernel are listed in /proc/filesystems
     */
    std::string type;

    /**
     * Device name, but can also be a directory name.
     */
    std::string source;

    /**
     * List of mount options in the fstab format.
     */
    std::vector<std::string> options;

    CARGO_REGISTER
    (
        type,
        source,
        options
    )
};

typedef std::map<std::string, MountRuntimeConfig> MountsRuntimeConfig;

} // namespace vasum


#endif // MOUNTS_CONFIG_HPP
