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
#include "lxcpp/container.hpp"
#include "lxcpp/exception.hpp"
#include "netlink/netlink-message.hpp"
#include "utils/make-clean.hpp"
#include "utils/text.hpp"
#include "logger/logger.hpp"

#include <iostream>

#include <linux/rtnetlink.h>
#include <net/if.h>

#define CHANGE_FLAGS_DEFAULT 0xffffffff
/* from linux/if_addr.h:
 * IFA_FLAGS is a u32 attribute that extends the u8 field ifa_flags.
 * If present, the value from struct ifaddrmsg will be ignored.
 *
 * But this FLAG is available since some kernel version
 *  check if one of extended flags id defined
 */
#ifndef IFA_F_MANAGETEMPADDR
#define IFA_FLAGS IFA_UNSPEC
#endif

using namespace vasum::netlink;

namespace lxcpp {

namespace {
std::string toString(const in_addr& addr)
{
    char buf[INET_ADDRSTRLEN];
    const char* ret = ::inet_ntop(AF_INET, &addr, buf, INET_ADDRSTRLEN);
    if (ret == NULL) {
        std::string msg = "Can't parse inet v4 addr";
        LOGE(msg);
        throw NetworkException(msg);
    }
    return ret;
}

std::string toString(const in6_addr& addr)
{
    char buf[INET6_ADDRSTRLEN];
    const char* ret = ::inet_ntop(AF_INET6, &addr, buf, INET6_ADDRSTRLEN);
    if (ret == NULL) {
        std::string msg = "Can't parse inet v6 addr";
        LOGE(msg);
        throw NetworkException(msg);
    }
    return ret;
}

uint32_t getInterfaceIndex(const std::string& name)
{
    uint32_t index = ::if_nametoindex(name.c_str());
    if (!index) {
        std::string msg = "Can't find interface";
        LOGE(msg);
        throw NetworkException(msg);
    }
    return index;
}

uint32_t getInterfaceIndex(const std::string& name, pid_t pid)
{
    NetlinkMessage nlm(RTM_GETLINK, NLM_F_REQUEST | NLM_F_ACK);
    ifinfomsg infoPeer = utils::make_clean<ifinfomsg>();
    infoPeer.ifi_family = AF_UNSPEC;
    infoPeer.ifi_change = CHANGE_FLAGS_DEFAULT;
    nlm.put(infoPeer)
        .put(IFLA_IFNAME, name);

    NetlinkResponse response = send(nlm, pid);
    if (!response.hasMessage()) {
        std::string msg = "Can't get interface index";
        LOGE(msg);
        throw NetworkException(msg);
    }

    response.fetch(infoPeer);
    return infoPeer.ifi_index;
}

void getAddressAttrs(Attrs& attrs, int family, const std::string& ifname, pid_t pid)
{
    uint32_t index = getInterfaceIndex(ifname, pid);
    NetlinkMessage nlm(RTM_GETADDR, NLM_F_REQUEST | NLM_F_ACK | NLM_F_DUMP);
    ifaddrmsg infoAddr = utils::make_clean<ifaddrmsg>();
    infoAddr.ifa_family = family; //test AF_PACKET to get all AF_INET* ?
    nlm.put(infoAddr);

    NetlinkResponse response = send(nlm, pid);
    if (!response.hasMessage()) {
        return ;
    }

    Attr attr;
    while (response.hasMessage()) {
        ifaddrmsg addrmsg;
        response.fetch(addrmsg);
        if (addrmsg.ifa_index == index) {
            InetAddr a;
            if (addrmsg.ifa_family == AF_INET6) {
                a.type = InetAddrType::IPV6;
            } else if (addrmsg.ifa_family == AF_INET) {
                a.type = InetAddrType::IPV4;
            } else {
                std::string msg = "Unsupported inet family";
                LOGE(msg);
                throw NetworkException(msg);
            }
            a.flags = addrmsg.ifa_flags; // IF_F_SECONDARY = secondary(alias)
            a.prefix = addrmsg.ifa_prefixlen;

            bool hasLocal = false;
            while (response.hasAttribute()) {

                int attrType = response.getAttributeType();
                switch (attrType) {

                // on each case line, int number in comment is value of the enum
                case IFA_ADDRESS: //1
                    if (hasLocal) {
                        response.skipAttribute();
                        break;
                    }
                    // fall thru
                case IFA_LOCAL: //2
                    if (addrmsg.ifa_family == AF_INET6) {
                        response.fetch(attrType, a.addr.ipv6);
                    } else if (addrmsg.ifa_family == AF_INET) {
                        response.fetch(attrType, a.addr.ipv4);
                    } else {
                        LOGW("unsupported family " << addrmsg.ifa_family);
                        response.skipAttribute();
                    }
                    hasLocal=true;
                    break;

                case IFA_FLAGS:  //8  extended flags - overwrites addrmsg.ifa_flags
                    response.fetch(IFA_FLAGS, a.flags);
                    break;

                case IFA_LABEL:    //3 <- should be equal to ifname
                case IFA_BROADCAST://4
                case IFA_ANYCAST:  //5
                case IFA_CACHEINFO://6
                case IFA_MULTICAST://7
                default:
                    //std::string tmp;
                    //response.fetch(attrType, tmp);
                    //std::cout << "skip(IFA) " << attrType << ":" << tmp << std::endl;
                    response.skipAttribute();
                    break;
                }
            }
            NetworkInterface::convertInetAddr2Attr(a, attr);
            attrs.push_back(attr);
        }
        response.fetchNextMessage();
    }
}
} // anonymous namespace


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

void NetworkInterface::move(const std::string& hostif)
{
     uint32_t index = getInterfaceIndex(hostif);
     NetlinkMessage nlm(RTM_NEWLINK, NLM_F_REQUEST | NLM_F_ACK);
     ifinfomsg infopeer = utils::make_clean<ifinfomsg>();
     infopeer.ifi_family = AF_UNSPEC;
     infopeer.ifi_index = index;
     nlm.put(infopeer)
        .put(IFLA_NET_NS_PID, mContainerPid);
     send(nlm);

     //rename to mIfname inside container
     if (mIfname != hostif) {
         renameFrom(hostif);
     }
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
    if (mContainerPid<=0) return CONFIGURED;
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

void NetworkInterface::renameFrom(const std::string& oldif)
{
    NetlinkMessage nlm(RTM_SETLINK, NLM_F_REQUEST | NLM_F_ACK);
    ifinfomsg infoPeer = utils::make_clean<ifinfomsg>();
    infoPeer.ifi_family = AF_UNSPEC;
    infoPeer.ifi_index = getInterfaceIndex(oldif, mContainerPid);
    infoPeer.ifi_change = CHANGE_FLAGS_DEFAULT;

    nlm.put(infoPeer)
        .put(IFLA_IFNAME, mIfname);
    send(nlm, mContainerPid);
}

void NetworkInterface::addInetAddr(const InetAddr& addr)
{
    NetlinkMessage nlm(RTM_NEWADDR, NLM_F_CREATE | NLM_F_REQUEST | NLM_F_ACK);
    ifaddrmsg infoAddr = utils::make_clean<ifaddrmsg>();
    infoAddr.ifa_index = getInterfaceIndex(mIfname, mContainerPid);
    infoAddr.ifa_family = addr.type == InetAddrType::IPV4 ? AF_INET : AF_INET6;
    infoAddr.ifa_prefixlen = addr.prefix;
    infoAddr.ifa_flags = addr.flags;
    nlm.put(infoAddr);

    if (addr.type == InetAddrType::IPV6) {
        nlm.put(IFA_ADDRESS, addr.addr.ipv6);
        nlm.put(IFA_LOCAL, addr.addr.ipv6);
    } else if (addr.type == InetAddrType::IPV4) {
        nlm.put(IFA_ADDRESS, addr.addr.ipv4);
        nlm.put(IFA_LOCAL, addr.addr.ipv4);
    }

    send(nlm, mContainerPid);
}

void NetworkInterface::setAttrs(const Attrs& attrs)
{
    //TODO check this: NetlinkMessage nlm(RTM_SETLINK, NLM_F_REQUEST | NLM_F_ACK);
    NetlinkMessage nlm(RTM_NEWLINK, NLM_F_REQUEST | NLM_F_CREATE | NLM_F_ACK);
    unsigned mtu=0, link=0, txq=0;
    ifinfomsg infoPeer = utils::make_clean<ifinfomsg>();
    infoPeer.ifi_family = AF_UNSPEC;
    infoPeer.ifi_index = getInterfaceIndex(mIfname, mContainerPid);
    infoPeer.ifi_change = CHANGE_FLAGS_DEFAULT;

    for (const auto& attr : attrs) {
        if (attr.name == AttrName::FLAGS) {
             infoPeer.ifi_flags = stoul(attr.value);
        } else if (attr.name == AttrName::CHANGE) {
            infoPeer.ifi_change = stoul(attr.value);
        } else if (attr.name == AttrName::TYPE) {
            infoPeer.ifi_type = stoul(attr.value);
        } else if (attr.name == AttrName::MTU) {
            mtu = stoul(attr.value);
        } else if (attr.name == AttrName::LINK) {
            link = stoul(attr.value);
        } else if (attr.name == AttrName::TXQLEN) {
            txq = stoul(attr.value);
        }
    }
    nlm.put(infoPeer);
    if (mtu) {
        nlm.put<uint32_t>(IFLA_MTU, mtu);
    }
    if (link) {
        nlm.put<uint32_t>(IFLA_LINK, link);
    }
    if (txq) {
        nlm.put<uint32_t>(IFLA_TXQLEN, txq);
    }

    NetlinkResponse response = send(nlm, mContainerPid);
    if (!response.hasMessage()) {
        throw NetworkException("Can't set interface information");
    }

    // configure inet addresses
    InetAddr addr;
    for (const Attr& a : attrs) {
        if (convertAttr2InetAddr(a, addr)) {
            addInetAddr(addr);
        }
    }
}

Attrs NetworkInterface::getAttrs() const
{
    NetlinkMessage nlm(RTM_GETLINK, NLM_F_REQUEST | NLM_F_ACK);
    ifinfomsg infoPeer = utils::make_clean<ifinfomsg>();
    infoPeer.ifi_family = AF_UNSPEC;
    infoPeer.ifi_change = CHANGE_FLAGS_DEFAULT;
    nlm.put(infoPeer)
        .put(IFLA_IFNAME, mIfname);

    NetlinkResponse response = send(nlm, mContainerPid);
    if (!response.hasMessage()) {
        throw NetworkException("Can't get interface information");
    }

    Attrs attrs;
    response.fetch(infoPeer);
    attrs.push_back(Attr{AttrName::FLAGS, std::to_string(infoPeer.ifi_flags)});
    attrs.push_back(Attr{AttrName::TYPE, std::to_string(infoPeer.ifi_type)});

    while (response.hasAttribute()) {
        /*
         * While traditional MAC addresses are all 48 bits in length,
         * a few types of networks require 64-bit addresses instead.
         */
        std::string mac;
        uint32_t mtu, link, txq;
        int attrType = response.getAttributeType();
        switch (attrType) {
        case IFLA_ADDRESS: //1
            response.fetch(IFLA_ADDRESS, mac);
            attrs.push_back(Attr{AttrName::MAC, utils::toHexString(mac.c_str(), mac.size())});
            break;
        case IFLA_MTU: //4
            response.fetch(IFLA_MTU, mtu);
            attrs.push_back(Attr{AttrName::MTU, std::to_string(mtu)});
            break;
        case IFLA_LINK://5
            response.fetch(IFLA_LINK, link);
            attrs.push_back(Attr{AttrName::LINK, std::to_string(link)});
            break;
        case IFLA_TXQLEN://13
            response.fetch(IFLA_TXQLEN, txq);
            attrs.push_back(Attr{AttrName::TXQLEN, std::to_string(txq)});
            break;
        case IFLA_BROADCAST://2 MAC broadcast
        case IFLA_IFNAME:   //3
        case IFLA_QDISC:    //6 queue discipline
        case IFLA_STATS:    //7 struct net_device_stats
        case IFLA_COST:     //8
        case IFLA_PRIORITY: //9
        case IFLA_MASTER:   //10
        case IFLA_WIRELESS: //11
        case IFLA_PROTINFO: //12
        case IFLA_MAP:      //14
        default:
            response.skipAttribute();
            break;
        }
    }
    getAddressAttrs(attrs, AF_INET, mIfname, mContainerPid);
    getAddressAttrs(attrs, AF_INET6, mIfname, mContainerPid);
    return attrs;
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
        // fetched value contains \0 terminator
        int len = response.getAttributeLength();
        response.fetch(IFLA_IFNAME, ifName, len - 1);
        iflist.push_back(ifName);
        response.fetchNextMessage();
    }
    return iflist;
}

void NetworkInterface::convertInetAddr2Attr(const InetAddr& a, Attr& attr)
{
    std::string value;

    if (a.type == InetAddrType::IPV6) {
        value += "ip=" + toString(a.addr.ipv6);
    } else if (a.type == InetAddrType::IPV4) {
        value += "ip=" + toString(a.addr.ipv4);
    } else {
        throw NetworkException();
    }

    value += ",pfx=" + std::to_string(a.prefix);
    value += ",flags=" + std::to_string(a.flags);

    if (a.type == InetAddrType::IPV6) {
        attr = Attr{AttrName::IPV6, value};
    } else {
        attr = Attr{AttrName::IPV4, value};
    }
}

bool NetworkInterface::convertAttr2InetAddr(const Attr& attr, InetAddr& addr)
{
    std::string::size_type s = 0U;
    std::string::size_type e = s;

    if (attr.name != AttrName::IPV6 && attr.name != AttrName::IPV4) {
        return false; //not inet attribute
    }

    addr.prefix = 0;
    addr.flags = 0;

    bool addrFound = false;
    while (e != std::string::npos) {
        e = attr.value.find(',', s);
        std::string ss = attr.value.substr(s, e - s);
        s = e + 1;

        std::string name;
        std::string value;
        std::string::size_type p = ss.find('=');
        if (p != std::string::npos) {
            name = ss.substr(0, p);
            value = ss.substr(p + 1);
        } else {
            name = ss;
            //value remains empty
        }

        if (name == "ip") {
            if (attr.name == AttrName::IPV6) {
                addr.type = InetAddrType::IPV6;
                if (inet_pton(AF_INET6, value.c_str(), &addr.addr.ipv6) != 1) {
                    throw NetworkException("Parse IPV6 address");
                }
            } else if (attr.name == AttrName::IPV4) {
                addr.type = InetAddrType::IPV4;
                if (inet_pton(AF_INET, value.c_str(), &addr.addr.ipv4) != 1) {
                    throw NetworkException("Parse IPV4 address");
                }
            }
            addrFound = true;
        } else if (name == "pfx") {
            addr.prefix = stoul(value);
        } else if (name == "flags") {
            addr.flags = stoul(value);
        }
    }

    return addrFound;
}

} // namespace lxcpp
