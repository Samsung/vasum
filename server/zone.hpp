/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Lukasz Pawelczyk <l.pawelczyk@partner.samsung.com>
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
 * @author  Lukasz Pawelczyk (l.pawelczyk@partner.samsung.com)
 * @brief   Declaration of the class for managing one zone
 */


#ifndef SERVER_ZONE_HPP
#define SERVER_ZONE_HPP

#include "zone-config.hpp"
#include "zone-provision.hpp"

#include "lxc/zone.hpp"
#include "netdev.hpp"

#include <mutex>
#include <string>
#include <memory>
#include <thread>


namespace vasum {


enum class SchedulerLevel {
    FOREGROUND,
    BACKGROUND
};

class Zone {

public:
    typedef netdev::Attrs NetdevAttrs;

    /**
     * Zone constructor
     * @param zoneId zone id
     * @param zonesPath directory where zones are defined (configs, rootfs etc)
     * @param zoneTemplatePath path for zones config template
     * @param dbPath path to dynamic config db file
     * @param zoneTemplateDir directory where templates are stored
     * @param baseRunMountPointPath base directory for run mount point
     */
    Zone(const std::string& zoneId,
         const std::string& zonesPath,
         const std::string& zoneTemplatePath,
         const std::string& dbPath,
         const std::string& zoneTemplateDir,
         const std::string& baseRunMountPointPath);
    Zone(const Zone&) = delete;
    Zone& operator=(const Zone&) = delete;
    ~Zone();

    typedef std::function<void(bool succeeded)> StartAsyncResultCallback;

    /**
     * Get the zone id
     */
    const std::string& getId() const;

    /**
     * Get the zone privilege
     */
    int getPrivilege() const;

    /**
     * Restore zone to the previous state
     */
    void restore();

    /**
     * Boot the zone to the background.
     */
    void start();

    /**
     * Try to shutdown the zone, if failed, destroy it.
     * @param saveState save zone's state
     */
    void stop(bool saveState);

    /**
     * Activate this zone's VT
     *
     * @return Was activation successful?
     */
    bool activateVT();

    /**
     * Setup this zone to be put in the foreground.
     * I.e. set appropriate scheduler level.
     */
    void goForeground();

    /**
     * Setup this zone to be put in the background.
     * I.e. set appropriate scheduler level.
     */
    void goBackground();

    /**
     * Set if zone should be detached on exit.
     */
    void setDetachOnExit();

    /**
     * Set if zone should be destroyed on exit.
     */
    void setDestroyOnExit();

    /**
     * @return Is the zone running?
     */
    bool isRunning();

    /**
     * Check if the zone is stopped. It's NOT equivalent to !isRunning,
     * because it checks different internal zone states. There are other states,
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
     * Resume zone.
     */
    void resume();

    /**
     * @return Is the zone in a paused state?
     */
    bool isPaused();

    /**
     * @return Is switching to default zone after timeout allowed?
     */
    bool isSwitchToDefaultAfterTimeoutAllowed() const;

    /**
     * Get id of VT
     */
    int getVT() const;

    /**
     * Create file inside zone, return its fd
     *
     * @param path  Path where the file should be created
     * @param flags Flags used when opening the file. See common/lxc/zone.hpp for more info.
     * @param mode  Permissions with which file is created
     *
     * @return Created files fd
     */
    int createFile(const std::string& path, const std::int32_t flags, const std::int32_t mode);

    /**
     * Declare file, directory or pipe that will be created while zone startup
     */
    std::string declareFile(const int32_t& type,
                            const std::string& path,
                            const int32_t& flags,
                            const int32_t& mode);
    /**
     * Declare mount that will be created while zone startup
     */
    std::string declareMount(const std::string& source,
                             const std::string& target,
                             const std::string& type,
                             const int64_t& flags,
                             const std::string& data);
    /**
     * Declare link that will be created while zone startup
     */
    std::string declareLink(const std::string& source,
                            const std::string& target);

    /**
     * Gets all declarations
     */
    std::vector<std::string> getDeclarations() const;

    /**
     * Remove declaration
     */
    void removeDeclaration(const std::string& declarationId);

    /**
     * Get zone root path
     */
    std::string getRootPath() const;

    /**
     * Create veth network device
     */
    void createNetdevVeth(const std::string& zoneDev,
                          const std::string& hostDev);

    /**
     * Create macvlan network device
     */
    void createNetdevMacvlan(const std::string& zoneDev,
                             const std::string& hostDev,
                             const uint32_t& mode);

    /**
     * Move network device to zone
     */
    void moveNetdev(const std::string& devId);

    /**
     * Destroy network device in zone
     */
    void destroyNetdev(const std::string& devId);

    /**
     * Set network device attributes
     */
    void setNetdevAttrs(const std::string& netdev, const NetdevAttrs& attrs);

    /**
     * Get network device attributes
     */
    NetdevAttrs getNetdevAttrs(const std::string& netdev);

    /**
     * Get network device list
     */
    std::vector<std::string> getNetdevList();

    /**
     * Remove ipv4/ipv6 address from network device
     */
    void deleteNetdevIpAddress(const std::string& netdev, const std::string& ip);

    /**
     * Sets the zones scheduler CFS quota.
     */
    void setSchedulerLevel(SchedulerLevel sched);

    /**
     * @return Scheduler CFS quota,
     * TODO: this function is only for UNIT TESTS
     */
    std::int64_t getSchedulerQuota();

private:
    ZoneConfig mConfig;
    ZoneDynamicConfig mDynamicConfig;
    std::unique_ptr<ZoneProvision> mProvision;
    mutable std::recursive_mutex mReconnectMutex;
    std::string mRunMountPoint;
    std::string mRootPath;
    std::string mDbPath;
    lxc::LxcZone mZone;
    const std::string mId;
    bool mDetachOnExit;
    bool mDestroyOnExit;

    void onNameLostCallback();
    void saveDynamicConfig();
    void updateRequestedState(const std::string& state);
    void setSchedulerParams(std::uint64_t cpuShares, std::uint64_t vcpuPeriod, std::int64_t vcpuQuota);
};


} // namespace vasum

#endif // SERVER_ZONE_HPP
