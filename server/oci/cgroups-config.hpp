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
 * @brief   CGroups configuration
 */

#ifndef CGROUPS_CONFIG_HPP
#define CGROUPS_CONFIG_HPP

#include "cargo/fields.hpp"

#include <string>
#include <vector>

namespace vasum {

struct CGroupMemoryConfig {

    /**
     * Memory limit (in bytes)
     */
    uint64_t limit;

    /**
     * Memory reservation or soft_limit (in bytes)
     */
    uint64_t reservation;

    /**
     * Total memory usage (memory + swap); set `-1' to disable swap
     */
    uint64_t swap;

    /**
     * Kernel memory limit (in bytes)
     */
    uint64_t kernel;

    /**
     * Tuning swappiness behaviour per cgroup
     */
    uint64_t swappiness;

    CARGO_REGISTER
    (
        limit,
        reservation,
        swap,
        kernel,
        swappiness
    )
};

struct CGroupCpuConfig {

    /**
     * specifies a relative share of CPU time available to the tasks in a cgroup
     */
    uint64_t shares;

    /**
     * specifies the total amount of time in microseconds
     * for which all tasks in a cgroup can run during one period
     * (as defined by period below)
     */
    uint64_t quota;

    /**
     * specifies a period of time in microseconds
     * for how regularly a cgroup's access to CPU resources should be reallocated
     * (CFS scheduler only)
     */
    uint64_t period;

    /**
     * specifies a period of time in microseconds
     * for the longest continuous period in which the tasks in a cgroup have access to CPU resources
     */
    uint64_t realtimeRuntime;

    /**
     * same as period but applies to realtime scheduler only
     */
    uint64_t realtimePeriod;

    /**
     * list of CPUs the container will run in
     */
    std::string cpus;

    /**
     * list of Memory Nodes the container will run in
     */
    std::string mems;

    CARGO_REGISTER
    (
        shares,
        quota,
        period,
        realtimeRuntime,
        realtimePeriod,
        cpus,
        mems
    )
};

struct CGroupPidsConfig {

    /**
     * specifies the maximum number of tasks in the cgroup
     */
    int64_t limit;

    CARGO_REGISTER
    (
        limit
    )
};

struct WeightDevice {

    /**
     * major, minor numbers for device
     */
    int64_t major;
    int64_t minor;

    /**
     * bandwidth rate for the device, range is from 10 to 1000
     */
    uint16_t weight;

    /**
     * bandwidth rate for the device while competing with the cgroup's child cgroups
     * range is from 10 to 1000, CFQ scheduler only
     */
    uint16_t leafWeight;

    CARGO_REGISTER
    (
        major,
        minor,
        weight,
        leafWeight
    )
};

struct ThrottleDevice {

    /**
     * major, minor numbers for device
     */
    int64_t major;
    int64_t minor;

    /**
     * IO rate limit for the device
     */
    uint64_t rate;

    CARGO_REGISTER
    (
        major,
        minor,
        rate
    )
};

struct CGroupBlockIOConfig {

    /**
     * This is default weight of the group on all devices until and unless overridden by per-device rules.
     * The range is from 10 to 1000.
     */
    uint16_t blkioWeight;

    /**
     * Equivalents of blkioWeight for the purpose of deciding
     * how much weight tasks in the given cgroup has while competing with the cgroup's child cgroups.
     * The range is from 10 to 1000.
     */
    uint16_t blkioLeafWeight;

    /**
     * Specifies the list of devices which will be bandwidth rate limited.
     */
    std::vector<WeightDevice> blkioWeightDevice;

    /**
     * Specify the list of devices which will be IO rate limited.
     */
    std::vector<ThrottleDevice> blkioThrottleWriteBpsDevice;
    std::vector<ThrottleDevice> blkioThrottleReadBpsDevice;
    std::vector<ThrottleDevice> blkioThrottleWriteIOPSDevice;
    std::vector<ThrottleDevice> blkioThrottleReadIOPSDevice;

    CARGO_REGISTER
    (
        blkioWeight,
        blkioLeafWeight,
        blkioWeightDevice,
        blkioThrottleWriteBpsDevice,
        blkioThrottleReadBpsDevice,
        blkioThrottleWriteIOPSDevice,
        blkioThrottleReadIOPSDevice
    )
};

struct HugePageLimit {

    /**
     * hugepage size
     */
    std::string pageSize;

    /**
     * limit in bytes of hugepagesize HugeTLB usage
     */
    uint64_t limit;

    CARGO_REGISTER
    (
        pageSize,
        limit
    )
};

typedef std::vector<HugePageLimit> CGroupHugePageLimitsConfig;

struct Priority {

    /**
     * interface name
     */
    std::string name;

    /**
     * priority applied to the interface
     */
    uint32_t priority;

    CARGO_REGISTER
    (
        name,
        priority
    )
};

struct CGroupNetworkConfig {

    /**
     * class identifier for container's network packets
     */
    std::string classID;

    /**
     * priorities of network traffic for container
     */
    std::vector<Priority> priorities;

    CARGO_REGISTER
    (
        classID,
        priorities
    )
};

} // namespace vasum


#endif // CGROUPS_CONFIG_HPP
