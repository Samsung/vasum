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
 * @brief   Declaration of the class for administrating one container
 */


#ifndef SERVER_CONTAINER_ADMIN_HPP
#define SERVER_CONTAINER_ADMIN_HPP

#include "container-config.hpp"
#include "lxc/zone.hpp"


namespace vasum {


enum class SchedulerLevel {
    FOREGROUND,
    BACKGROUND
};

class ContainerAdmin {

public:

    /**
     * ContainerAdmin constructor
     * @param containersPath directory where containers are defined (lxc configs, rootfs etc)
     * @param lxcTemplatePrefix directory where templates are stored
     * @param config containers config
     */
    ContainerAdmin(const std::string& containersPath,
                   const std::string& lxcTemplatePrefix,
                   const ContainerConfig& config);
    virtual ~ContainerAdmin();

    /**
     * Get the container id
     */
    const std::string& getId() const;

    /**
     * Boot the container to the background.
     */
    void start();

    /**
     * Try to shutdown the container, if failed, kill it.
     */
    void stop();

    /**
     * Destroy stopped container. In particular it removes whole containers rootfs.
     */
    void destroy();

    /**
     * @return Is the container running?
     */
    bool isRunning();

    /**
     * Check if the container is stopped. It's NOT equivalent to !isRunning,
     * because it checks different internal lxc states. There are other states,
     * (e.g. paused) when the container isn't running nor stopped.
     *
     * @return Is the container stopped?
     */
    bool isStopped();

    /**
     * Suspends an active container, the process is frozen
     * without further access to CPU resources and I/O,
     * but the memory used by the container
     * at the hypervisor level will stay allocated
     */
    void suspend();

    /**
     * Resume the container after suspension.
     */
    void resume();

    /**
     * @return Is the container in a paused state?
     */
    bool isPaused();

    /**
     * Sets the containers scheduler CFS quota.
     */
    void setSchedulerLevel(SchedulerLevel sched);

    /**
     * Set whether container should be detached on exit.
     */
    void setDetachOnExit();

    /**
     * Set if container should be destroyed on exit.
     */
    void setDestroyOnExit();

    /**
     * @return Scheduler CFS quota,
     * TODO: this function is only for UNIT TESTS
     */
    std::int64_t getSchedulerQuota();

private:
    const ContainerConfig& mConfig;
    lxc::LxcZone mZone;
    const std::string mId;
    bool mDetachOnExit;
    bool mDestroyOnExit;

    void setSchedulerParams(std::uint64_t cpuShares, std::uint64_t vcpuPeriod, std::int64_t vcpuQuota);
};


} // namespace vasum


#endif // SERVER_CONTAINER_ADMIN_HPP
