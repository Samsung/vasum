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

#include "lxcpp/commands/netcreate.hpp"
#include "lxcpp/network.hpp"

namespace lxcpp {

void NetCreateAll::execute()
{
    pid_t pid = mContainer.getInitPid();
    for (const auto& interface : mNetwork.getInterfaces()) {
        NetworkInterface networkInterface(interface.getZoneIf(), pid);
        networkInterface.create(interface.getType(), interface.getHostIf(), interface.getMode());

        Attrs attrs;
        if (interface.getMTU() > 0) {
            attrs.push_back(Attr{AttrName::MTU, std::to_string(interface.getMTU())});
        }
        if (!interface.getMACAddress().empty()) {
            attrs.push_back(Attr{AttrName::MAC, interface.getMACAddress()});
        }
        if (interface.getTxLength() > 0) {
            attrs.push_back(Attr{AttrName::TXQLEN, std::to_string(interface.getTxLength())});
        }
        networkInterface.setAttrs(attrs);

        for (const auto& addr : interface.getAddrList()) {
            networkInterface.addInetAddr(addr);
        }
    }
}

void NetInteraceCreate::execute()
{
    NetworkInterface networkInterface(mZoneIf, mContainer.getInitPid());
    networkInterface.create(mType, mHostIf, mMode);
}

void NetInterfaceSetAttrs::execute()
{
    NetworkInterface networkInterface(mIfname, mContainer.getInitPid());
    networkInterface.setAttrs(mAttrs);
}

void NetInterfaceAddInetAddr::execute()
{
    NetworkInterface networkInterface(mIfname, mContainer.getInitPid());
    for (const auto& a : mAddrList) {
        networkInterface.addInetAddr(a);
    }
}

} // namespace lxcpp
