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
    virtual ~Container() {};

    // Configuration
    virtual const std::string& getName() const = 0;
    virtual const std::string& getRootPath() const = 0;

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

    // Execution actions
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void freeze() = 0;
    virtual void unfreeze() = 0;
    virtual void reboot() = 0;

    // Other
    virtual void attach(const std::vector<std::string>& argv,
                        const std::string& cwdInContainer) = 0;
    virtual void console() = 0;
    virtual bool isRunning() const = 0;

    // Network interfaces setup/config
    virtual void addInterfaceConfig(const std::string& hostif,
                                    const std::string& zoneif,
                                    InterfaceType type,
                                    const std::vector<InetAddr>& addrs,
                                    MacVLanMode mode) = 0;
    virtual void addInetConfig(const std::string& ifname, const InetAddr& addr) = 0;

    // Network interfaces (runtime)
    virtual std::vector<std::string> getInterfaces() const = 0;
    virtual NetworkInterfaceInfo getInterfaceInfo(const std::string& ifname) const = 0;
    virtual void createInterface(const std::string& hostif,
                                 const std::string& zoneif,
                                 InterfaceType type,
                                 MacVLanMode mode) = 0;
    virtual void destroyInterface(const std::string& ifname) = 0;
    virtual void moveInterface(const std::string& ifname) = 0;
    virtual void setUp(const std::string& ifname) = 0;
    virtual void setDown(const std::string& ifname) = 0;
    virtual void addInetAddr(const std::string& ifname, const InetAddr& addr) = 0;
    virtual void delInetAddr(const std::string& ifname, const InetAddr& addr) = 0;

    // Provisioning
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
};

} // namespace lxcpp

#endif // LXCPP_CONTAINER_HPP
