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
 * @brief   A definition of a container config
 */

#ifndef CONTAINER_CONFIG_HPP
#define CONTAINER_CONFIG_HPP

#include "config.hpp"
#include "cargo/fields.hpp"

#include "devices-config.hpp"
#include "hooks-config.hpp"
#include "mounts-config.hpp"
#include "namespaces-config.hpp"
#include "process-config.hpp"
#include "resources-config.hpp"

namespace vasum {

struct PlatformConfig {

    /**
     * Specifies the operating system family this image must run on.
     */
    std::string os;

    /**
     * Specifies the instruction set for which the binaries in the image have been compiled.
     */
    std::string arch;

    CARGO_REGISTER
    (
        os,
        arch
    )
};

struct RootConfig {

    /**
     * Specifies the path to the root filesystem for the container
     * relative to the path where the manifest is.
     */
    std::string path;

    /**
     * If true then the root filesystem MUST be read-only inside the container.
     */
    bool readonly;

    CARGO_REGISTER
    (
        path,
        readonly
    )
};

struct LinuxConfig {

    std::vector<std::string> capabilities;

    CARGO_REGISTER
    (
        capabilities
    )
};

struct ContainerConfig {

    std::string version;
    PlatformConfig platform;
    ProcessConfig process;
    RootConfig root;
    std::string hostname;
    MountsConfig mounts;
    LinuxConfig linux;

    CARGO_REGISTER
    (
        version,
        platform,
        process,
        root,
        hostname,
        mounts,
        linux
    )
};

struct LinuxRuntimeConfig {

    IDMapConfig uidMappings;
    IDMapConfig gidMappings;
    RlimitsConfig rlimits;
    std::map<std::string, std::string> sysctl;
    ResourcesConfig resources;
    std::string cgroupsPath;
    NamespacesConfig namespaces;
    DevicesConfig devices;

    CARGO_REGISTER
    (
        uidMappings,
        gidMappings,
        rlimits,
        sysctl,
        resources,
        cgroupsPath,
        namespaces,
        devices
    )
};

struct ContainerRuntimeConfig {

    MountsRuntimeConfig mounts;
    HooksConfig hooks;
    LinuxRuntimeConfig linux;

    CARGO_REGISTER
    (
        mounts,
        hooks,
        linux
    )
};

} // namespace vasum


#endif // CONTAINER_CONFIG_HPP
