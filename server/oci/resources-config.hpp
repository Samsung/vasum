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
 * @brief   Resources configuration
 */

#ifndef RESOURCES_CONFIG_HPP
#define RESOURCES_CONFIG_HPP

#include "config.hpp"
#include "cargo/fields.hpp"

#include "cgroups-config.hpp"

namespace vasum {

struct Rlimit {

    /**
     * value from those defined in http://man7.org/linux/man-pages/man2/setrlimit.2.html
     */
    std::string type;

    /**
     * Kernel enforces the soft limit for a resource,
     * the hard limit acts as a ceiling for that value that could be set by an unprivileged process.
     */
    uint64_t hard;
    uint64_t soft;

    CARGO_REGISTER
    (
        type,
        hard,
        soft
    )
};

typedef std::vector<Rlimit> RlimitsConfig;

struct ResourcesConfig {

    bool disableOOMKiller;
    CGroupMemoryConfig memory;
    CGroupCpuConfig cpu;
    CGroupPidsConfig pids;
    CGroupBlockIOConfig blockIO;
    CGroupHugePageLimitsConfig hugepageLimits;
    CGroupNetworkConfig network;

    CARGO_REGISTER
    (
        disableOOMKiller,
        memory,
        cpu,
        pids,
        blockIO,
        hugepageLimits,
        network
    )
};

} // namespace vasum


#endif // RESOURCES_CONFIG_HPP
