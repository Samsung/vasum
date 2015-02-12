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
 * @brief   Declaration of the class for administrating one zone
 */


#ifndef SERVER_ZONE_ADMIN_HPP
#define SERVER_ZONE_ADMIN_HPP

#include "zone-config.hpp"
#include "lxc/zone.hpp"


namespace vasum {


enum class SchedulerLevel {
    FOREGROUND,
    BACKGROUND
};

class ZoneAdmin {

public:

    /**
     * ZoneAdmin constructor
     * @param zoneId zone id
     * @param zonesPath directory where zones are defined (lxc configs, rootfs etc)
     * @param lxcTemplatePrefix directory where templates are stored
     * @param config zones config
     * @param dynamicConfig zones dynamic config
     */
    ZoneAdmin(const std::string& zoneId,
              const std::string& zonesPath,
              const std::string& lxcTemplatePrefix,
              const ZoneConfig& config,
              const ZoneDynamicConfig& dynamicConfig);
    virtual ~ZoneAdmin();

    /**
     * Get the zone id
     */
    const std::string& getId() const;

    /**
     * Boot the zone to the background.
     */
    void start();

    /**
     * Try to shutdown the zone, if failed, kill it.
     */
    void stop();

    /**
     * Destroy stopped zone. In particular it removes whole zones rootfs.
     */
    void destroy();

    /**
     * @return Is the zone running?
     */
    bool isRunning();

    /**
     * Check if the zone is stopped. It's NOT equivalent to !isRunning,
     * because it checks different internal lxc states. There are other states,
     * (e.g. paused) when the zone isn't running nor stopped.
     *
     * @return Is the zone stopped?
     */
    bool isStopped();

    /**
     * Suspends an active zone, the process is frozen
     * without further access to CPU resources and I/O,
     * but the memory used by the zone
     * at the hypervisor level will stay allocated
     */
    void suspend();

    /**
     * Resume the zone after suspension.
     */
    void resume();

    /**
     * @return Is the zone in a paused state?
     */
    bool isPaused();

    /**
     * Sets the zones scheduler CFS quota.
     */
    void setSchedulerLevel(SchedulerLevel sched);

    /**
     * Set whether zone should be detached on exit.
     */
    void setDetachOnExit();

    /**
     * Set if zone should be destroyed on exit.
     */
    void setDestroyOnExit();

    /**
     * @return Scheduler CFS quota,
     * TODO: this function is only for UNIT TESTS
     */
    std::int64_t getSchedulerQuota();

private:
    const ZoneConfig& mConfig;
    lxc::LxcZone mZone;
    const std::string mId;
    bool mDetachOnExit;
    bool mDestroyOnExit;

    void setSchedulerParams(std::uint64_t cpuShares, std::uint64_t vcpuPeriod, std::int64_t vcpuQuota);
};


} // namespace vasum


#endif // SERVER_ZONE_ADMIN_HPP
