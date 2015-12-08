/*
 *  Copyright (C) 2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License version 2.1 as published by the Free Software Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

/**
 * @file
 * @author  Mateusz Malicki (m.malicki2@samsung.com)
 * @brief   Container interface
 */

#ifndef LXCPP_CONTAINER_HPP
#define LXCPP_CONTAINER_HPP

#include "lxcpp/network-config.hpp"
#include "lxcpp/provision-config.hpp"
#include "lxcpp/cgroups/cgroup-config.hpp"
#include "lxcpp/logger-config.hpp"

#include <sys/types.h>

#include <string>
#include <functional>
#include <vector>

namespace lxcpp {

struct NetworkInterfaceInfo {
    const std::string ifname;
    const NetStatus status;
    const std::string macaddr;
    const int mtu;
    const int flags;
    const std::vector<InetAddr> addrs;
};

class Container {
public:
    typedef std::function<void(void)> Callback;

    enum class State: int {
        STOPPED,   //< Init isn't running
        STOPPING,  //< Init's stop is triggered
        STARTING,  //< Container is being set up
        RUNNING,   //< Init in container is running
        CONNECTING //< Synchronizing with existing Guard
    };

    virtual ~Container() {};

    /**
     * Configuration
     */
    virtual const std::string& getName() const = 0;
    virtual const std::string& getRootPath() const = 0;
    virtual void setHostName(const std::string& hostname) = 0;

    virtual pid_t getGuardPid() const = 0;
    virtual pid_t getInitPid() const = 0;

    virtual const std::vector<std::string>& getInit() = 0;
    virtual void setInit(const std::vector<std::string> &init) = 0;

    virtual void setLogger(const logger::LogType type,
                           const logger::LogLevel level,
                           const std::string &arg = "") = 0;

    virtual void setTerminalCount(const unsigned int count) = 0;

    virtual void addUIDMap(unsigned min, unsigned max, unsigned num) = 0;
    virtual void addGIDMap(unsigned min, unsigned max, unsigned num) = 0;

    /**
     * Execution actions
     */
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void freeze() = 0;
    virtual void unfreeze() = 0;
    virtual void reboot() = 0;
    virtual bool connect() = 0;

    /**
     * States
     */
    virtual State getState() = 0;
    virtual void setStartedCallback(const Callback& callback) = 0;
    virtual void setStoppedCallback(const Callback& callback) = 0;

    /**
     * Other
     */
    virtual int attach(const std::vector<std::string>& argv,
                       const uid_t uid,
                       const gid_t gid,
                       const std::string& ttyPath,
                       const std::vector<gid_t>& supplementaryGids,
                       const int capsToKeep,
                       const std::string& workDirInContainer,
                       const std::vector<std::string>& envToKeep,
                       const std::vector<std::pair<std::string, std::string>>& envToSet) = 0;
    virtual void console() = 0;

    // Network interfaces setup/config
    virtual void addInterfaceConfig(InterfaceConfigType type,
                                    const std::string& hostif,
                                    const std::string& zoneif = "",
                                    const std::vector<InetAddr>& addrs = std::vector<InetAddr>(),
                                    MacVLanMode mode = MacVLanMode::PRIVATE) = 0;
    virtual void addInetConfig(const std::string& ifname, const InetAddr& addr) = 0;

    /**
     * Network interfaces (runtime)
     */
    virtual std::vector<std::string> getInterfaces() const = 0;
    virtual NetworkInterfaceInfo getInterfaceInfo(const std::string& ifname) const = 0;
    virtual void createInterface(const std::string& hostif,
                                 const std::string& zoneif,
                                 InterfaceType type,
                                 MacVLanMode mode) = 0;
    virtual void destroyInterface(const std::string& ifname) = 0;
    virtual void moveInterface(const std::string& ifname) = 0;
    virtual void setUpInterface(const std::string& ifname) = 0;
    virtual void setDownInterface(const std::string& ifname) = 0;
    virtual void addInetAddr(const std::string& ifname, const InetAddr& addr) = 0;
    virtual void delInetAddr(const std::string& ifname, const InetAddr& addr) = 0;

    /**
     * Provisioning
     */
    virtual void declareFile(const provision::File::Type type,
                             const std::string& path,
                             const int32_t flags,
                             const int32_t mode) = 0;
    virtual const FileVector& getFiles() const = 0;
    virtual void removeFile(const provision::File& item) = 0;

    virtual void declareMount(const std::string& source,
                              const std::string& target,
                              const std::string& type,
                              const int64_t flags,
                              const std::string& data) = 0;
    virtual const MountVector& getMounts() const = 0;
    virtual void removeMount(const provision::Mount& item) = 0;

    virtual void declareLink(const std::string& source,
                             const std::string& target) = 0;
    virtual const LinkVector& getLinks() const = 0;
    virtual void removeLink(const provision::Link& item) = 0;

    /**
     * CGroups
     */
    virtual void addSubsystem(const std::string& name, const std::string& path) = 0;
    virtual void addCGroup(const std::string& subsys,
                           const std::string& grpname,
                           const std::vector<CGroupParam>& comm,
                           const std::vector<CGroupParam>& params) = 0;

    /**
     * Environment variables
     */
    virtual void setEnv(const std::vector<std::pair<std::string, std::string>>& variables) = 0;

    /**
     * Linux capabilities
     */
    virtual void setCaps(const int caps) = 0;

    /**
     * System Property (sysctl)
     */
    virtual void setSystemProperty(const std::string& name, const std::string& value) = 0;

    /**
     * Rlimit
     */
    virtual void setRlimit(const std::string& type, const uint64_t hard, const uint64_t soft) = 0;

    /**
     * Namespaces
     * TODO Needed to implement application container.
     */
    virtual void setNamespaces(const int namespaces) = 0;

    /**
     * UID/GIDS
     * TODO Needed to implement application container.
     */
    virtual void setUser(const int uid, const int gid, const std::vector<int> additionalGids) = 0;

    /**
     * Device
     */
    virtual void addDevice(const std::string& path,
                           const char type,
                           const int64_t major,
                           const int64_t minor,
                           const std::string& permissions,
                           const uint32_t fileMode,
                           const uint32_t uid,
                           const uint32_t gid) = 0;

    /**
     * Hooks
     */
    virtual void addHook(const std::string& type,
                         const std::vector<std::string>& hook,
                         const std::vector<std::pair<std::string, std::string>>& env) = 0;

};

} // namespace lxcpp

#endif // LXCPP_CONTAINER_HPP
