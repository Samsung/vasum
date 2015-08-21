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
 * @brief   Actions on network interace in the container
 */


#include "lxcpp/network.hpp"
#include "lxcpp/exception.hpp"
#include "netlink/netlink-message.hpp"
#include "utils/make-clean.hpp"

#include <linux/rtnetlink.h>

using namespace vasum::netlink;

namespace lxcpp {

void NetworkInterface::create(const std::string& hostif,
                              InterfaceType type,
                              MacVLanMode mode)
{
    switch (type) {
        case InterfaceType::VETH:
            createVeth(hostif);
            break;
        case InterfaceType::BRIDGE:
            createBridge(hostif);
            break;
        case InterfaceType::MACVLAN:
            createMacVLan(hostif, mode);
            break;
        case InterfaceType::MOVE:
            move(hostif);
            break;
        default:
            throw NetworkException("Unsuported interface type");
    }
}

void NetworkInterface::createVeth(const std::string& /*hostif*/)
{
    throw NotImplementedException();
}

void NetworkInterface::createBridge(const std::string& /*hostif*/)
{
    throw NotImplementedException();
}

void NetworkInterface::createMacVLan(const std::string& /*hostif*/, MacVLanMode /*mode*/)
{
    throw NotImplementedException();
}

void NetworkInterface::move(const std::string& /*hostif*/)
{
    throw NotImplementedException();
}

void NetworkInterface::destroy()
{
    throw NotImplementedException();
}

NetStatus NetworkInterface::status()
{
    throw NotImplementedException();
    /*
    //TODO get container status, if stopped return CONFIGURED
    if (mContainer.getInitPid()<=0) return CONFIGURED;
    // read netlink
    return DOWN;*/
}


void NetworkInterface::up()
{
    throw NotImplementedException();
}

void NetworkInterface::down()
{
    throw NotImplementedException();
}

void NetworkInterface::setAttrs(const Attrs& /*attrs*/)
{
    throw NotImplementedException();
}

const Attrs NetworkInterface::getAttrs() const
{
    throw NotImplementedException();
}

std::vector<std::string> NetworkInterface::getInterfaces(pid_t initpid)
{
    // get interfaces seen by netlink
    NetlinkMessage nlm(RTM_GETLINK, NLM_F_REQUEST|NLM_F_DUMP|NLM_F_ROOT);
    ifinfomsg info = utils::make_clean<ifinfomsg>();
    info.ifi_family = AF_PACKET;
    nlm.put(info);
    NetlinkResponse response = send(nlm, initpid);

    std::vector<std::string> iflist;
    while (response.hasMessage()) {
        std::string ifName;
        response.skip<ifinfomsg>();
        response.fetch(IFLA_IFNAME, ifName);
        iflist.push_back(ifName);
        response.fetchNextMessage();
    }
    return iflist;
}

} // namespace lxcpp
