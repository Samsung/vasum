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
 * @brief   Namespaces configuration
 */

#ifndef NAMESPACES_CONFIG_HPP
#define NAMESPACES_CONFIG_HPP

#include "cargo/fields.hpp"

#include <string>
#include <vector>

namespace vasum {

struct NamespaceConfig {

    /**
     * Namespace type.
     * The following namespaces types are supported: pid, network, mount, ipc, uts, user.
     */
    std::string type;

    /**
     * Path to namespace file.
     */
    std::string path;

    CARGO_REGISTER
    (
        type,
        path
    )
};

typedef std::vector<NamespaceConfig> NamespacesConfig;

struct IDMap {

    /**
     * Describe the user namespace mappings from the host to the container.
     * hostID is the starting uid/gid on the host to be mapped to containerID which is the starting uid/gid in the container
     * size refers to the number of ids to be mapped
     */
    int hostID;
    int containerID;
    int size;

    CARGO_REGISTER
    (
        hostID,
        containerID,
        size
    )
};

typedef std::vector<IDMap> IDMapConfig;

} // namespace vasum


#endif // NAMESPACES_CONFIG_HPP
