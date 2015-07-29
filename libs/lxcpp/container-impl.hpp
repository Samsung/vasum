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
 * @brief   ContainerImpl main class
 */

#ifndef LXCPP_CONTAINER_IMPL_HPP
#define LXCPP_CONTAINER_IMPL_HPP

#include <sys/types.h>
#include <config/config.hpp>
#include <config/fields.hpp>
#include <memory>

#include "lxcpp/container.hpp"
#include "lxcpp/namespace.hpp"
#include "lxcpp/network.hpp"

#include "utils/channel.hpp"

namespace lxcpp {

struct ContainerConfig {
    std::string mName;
    std::string mRootPath;
    pid_t mGuardPid;
    pid_t mInitPid;
    std::vector<std::string> mInit;

    ContainerConfig() : mGuardPid(-1), mInitPid(-1) {}

    CONFIG_REGISTER
    (
        mName,
        mRootPath,
        mGuardPid,
        mInitPid,
        mInit
    )
};


class ContainerImpl : public virtual Container {
public:
    ContainerImpl(const std::string &name, const std::string &path);
    ContainerImpl(pid_t guardPid);
    ~ContainerImpl();

    // Configuration
    const std::string& getName() const;
    const std::string& getRootPath() const;

    pid_t getGuardPid() const;
    pid_t getInitPid() const;

    const std::vector<std::string>& getInit();
    void setInit(const std::vector<std::string> &init);

    const std::vector<Namespace>& getNamespaces() const;

    // Execution actions
    void start();
    void stop();
    void freeze();
    void unfreeze();
    void reboot();

    // Other
    void attach(Container::AttachCall& attachCall,
                const std::string& cwdInContainer);

    // Network interfaces setup/config
    void addInterfaceConfig(const std::string& hostif,
                         const std::string& zoneif,
                         InterfaceType type,
                         MacVLanMode mode);
    void addAddrConfig(const std::string& ifname, const InetAddr& addr);

    // Network interfaces (runtime)
    std::vector<std::string> getInterfaces();
    NetworkInterfaceInfo getInterfaceInfo(const std::string& ifname);
    void createInterface(const std::string& hostif,
                         const std::string& zoneif,
                         InterfaceType type,
                         MacVLanMode mode);
    void destroyInterface(const std::string& ifname);
    void setUp(const std::string& ifname);
    void setDown(const std::string& ifname);
    void addAddr(const std::string& ifname, const InetAddr& addr);
    void delAddr(const std::string& ifname, const InetAddr& addr);

private:
    ContainerConfig mConfig;

    // TODO: convert to ContainerConfig struct
    std::vector<Namespace> mNamespaces;
    std::vector<NetworkInterfaceConfig> mInterfaceConfig;
};

} // namespace lxcpp

#endif // LXCPP_CONTAINER_IMPL_HPP
