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
 * @author  Krzysztof Dynowski (k.dynowski@samsumg.com)
 * @brief   Network configuration command
 */

#ifndef LXCPP_COMMAND_NETCREATE_HPP
#define LXCPP_COMMAND_NETCREATE_HPP

#include "lxcpp/commands/command.hpp"
#include "lxcpp/network-config.hpp"
#include "lxcpp/container.hpp"

#include <sys/types.h>

namespace lxcpp {

class NetCreateAll final: Command {
public:
   /**
    * Runs call in the container's context
    *
    * Creates and configures network interfaces in the container
    */
    NetCreateAll(const Container& container, const NetworkConfig& network) :
        mContainer(container),
        mNetwork(network)
    {
    }

    void execute();

private:
    const Container& mContainer;
    const NetworkConfig& mNetwork;
};

class NetInteraceCreate final: Command {
public:
    NetInteraceCreate(const Container& container,
                      const std::string& zoneif,
                      const std::string& hostif,
                      InterfaceType type,
                      MacVLanMode mode=MacVLanMode::PRIVATE) :
        mContainer(container),
        mZoneIf(zoneif),
        mHostIf(hostif),
        mType(type),
        mMode(mode)
    {
    }

    void execute();

private:
    const Container& mContainer;
    const std::string& mZoneIf;
    const std::string& mHostIf;
    InterfaceType mType;
    MacVLanMode mMode;
};

class NetInterfaceSetAttrs final: Command {
public:
    NetInterfaceSetAttrs(const Container& container,
                         const std::string& ifname,
                         const Attrs& attrs) :
        mContainer(container),
        mIfname(ifname),
        mAttrs(attrs)
    {
    }

    void execute();

private:
    const Container& mContainer;
    const std::string& mIfname;
    const Attrs& mAttrs;
};

class NetInterfaceAddInetAddr final: Command {
public:
    NetInterfaceAddInetAddr(const Container& container,
                            const std::string& ifname,
                            const std::vector<InetAddr>& addrList) :
        mContainer(container),
        mIfname(ifname),
        mAddrList(addrList)
    {
    }

    void execute();

private:
    const Container& mContainer;
    const std::string& mIfname;
    const std::vector<InetAddr>& mAddrList;
};

} // namespace lxcpp

#endif // LXCPP_COMMAND_NETCREATE_HPP
