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

#include <sys/types.h>

namespace lxcpp {

class NetCreateAll final: Command {
public:
   /**
    * Runs call in the container's context
    *
    * Creates and configures network interfaces in the container
    *
    */
    NetCreateAll(pid_t containerPid, const std::vector<NetworkInterfaceConfig>& iflist) :
        mContainerPid(containerPid),
        mInterfaceConfigs(iflist)
    {
    }

    void execute();

private:
    const pid_t mContainerPid;
    const std::vector<NetworkInterfaceConfig>& mInterfaceConfigs;
};

} // namespace lxcpp

#endif // LXCPP_COMMAND_NETCREATE_HPP
