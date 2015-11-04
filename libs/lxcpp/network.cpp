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
#include "utils/exception.hpp"
#include "logger/logger.hpp"

#include <iostream>

#include <unistd.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <linux/rtnetlink.h>
#include <linux/veth.h>
#include <linux/sockios.h>
#include <linux/if_bridge.h>

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

uint32_t getInterfaceIndex(const std::string& name)
{
    uint32_t index = ::if_nametoindex(name.c_str());
    if (!index) {
        const std::string msg = "Can't find interface " + utils::getSystemErrorMessage();
        LOGE(msg);
        throw NetworkException(msg);
    }
    return index;
}

std::string getInterfaceName(uint32_t index)
{
    char buf[IFNAMSIZ];
    if (::if_indextoname(index, buf) == NULL) {
        const std::string msg = "No interface for index: " + std::to_string(index);
        LOGE(msg);
        throw NetworkException(msg);
    }
    return buf;
}

uint32_t getInterfaceIndex(pid_t pid, const std::string& name)
{
    NetlinkMessage nlm(RTM_GETLINK, NLM_F_REQUEST | NLM_F_ACK);
    ifinfomsg info = utils::make_clean<ifinfomsg>();
    info.ifi_family = AF_UNSPEC;
    info.ifi_change = CHANGE_FLAGS_DEFAULT;
    nlm.put(info)
        .put(IFLA_IFNAME, name);

    NetlinkResponse response = send(nlm, pid);
    if (!response.hasMessage()) {
        const std::string msg = "Can't get interface index for " + name;
        LOGE(msg);
        throw NetworkException(msg);
    }

    response.fetch(info);
    return info.ifi_index;
}

void bridgeModify(pid_t pid, const std::string& ifname, uint32_t masterIndex) {
    NetlinkMessage nlm(RTM_SETLINK, NLM_F_REQUEST | NLM_F_ACK);
    ifinfomsg info = utils::make_clean<ifinfomsg>();
    info.ifi_family = AF_UNSPEC;
    info.ifi_change = CHANGE_FLAGS_DEFAULT;
    info.ifi_index = getInterfaceIndex(ifname);
    nlm.put(info)
        .put(IFLA_MASTER, masterIndex);
    send(nlm, pid);
}

void getAddressList(pid_t pid, std::vector<InetAddr>& addrs, int family, const std::string& ifname)
{
    uint32_t index = getInterfaceIndex(pid, ifname);
    NetlinkMessage nlm(RTM_GETADDR, NLM_F_REQUEST | NLM_F_ACK | NLM_F_DUMP);
    ifaddrmsg infoAddr = utils::make_clean<ifaddrmsg>();
    infoAddr.ifa_family = family; //test AF_PACKET to get all AF_INET* ?
    nlm.put(infoAddr);

    NetlinkResponse response = send(nlm, pid);
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
                const std::string msg = "Unsupported inet family";
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
                        response.fetch(attrType, a.getAddr<in6_addr>());
                    } else if (addrmsg.ifa_family == AF_INET) {
                        response.fetch(attrType, a.getAddr<in_addr>());
                    } else {
                        LOGW("unsupported family " << addrmsg.ifa_family);
                        response.skipAttribute();
                    }
                    hasLocal = true;
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
            addrs.push_back(a);
        }
        response.fetchNextMessage();
    }
}

/*
 * convert string-hex to byte array
 */
void toMacAddressArray(const std::string& s, char *buf, size_t bs)
{
    static const std::string hex = "0123456789ABCDEF";
    int nibble = 0; //nibble counter, one nibble = 4 bits (or one hex digit)
    int v = 0;
    ::memset(buf, 0, bs);
    for (size_t i = 0; i < s.length() && bs > 0; ++i) {
        const char c = s.at(i);
        size_t pos = hex.find(c);
        if (pos == std::string::npos) { // ignore eny non-hex character
            continue;
        }
        v <<= 4;
        v |= pos; // add nibble
        if (++nibble == 2) { // two nibbles collected (one byte)
            *buf++ = v; // assign the byte
            --bs;       // decrease space left
            v = 0;      // reset byte and nibble counter
            nibble = 0;
        }
    }
}

} // anonymous namespace


std::string toString(const in_addr& addr)
{
    char buf[INET_ADDRSTRLEN];
    const char* ret = ::inet_ntop(AF_INET, &addr, buf, INET_ADDRSTRLEN);
    if (ret == NULL) {
        const std::string msg = "Wrong inet v4 addr " + utils::getSystemErrorMessage();
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
        const std::string msg = "Wrong inet v6 addr " + utils::getSystemErrorMessage();
        LOGE(msg);
        throw NetworkException(msg);
    }
    return ret;
}

void fromString(const std::string& s, in_addr& addr)
{
    if (s.empty()) {
        ::memset(&addr, 0, sizeof(addr));
    }
    else if (::inet_pton(AF_INET, s.c_str(), &addr) != 1) {
        const std::string msg = "Can't parse inet v4 addr " + utils::getSystemErrorMessage();
        LOGE(msg);
        throw NetworkException(msg);
    }
}

void fromString(const std::string& s, in6_addr& addr)
{
    if (s == ":") {
        ::memset(&addr, 0, sizeof(addr));
    }
    else if (::inet_pton(AF_INET6, s.c_str(), &addr) != 1) {
        const std::string msg = "Can't parse inet v6 addr " + utils::getSystemErrorMessage();
        LOGE(msg);
        throw NetworkException(msg);
    }
}

std::string toString(const InetAddr& a) {
    std::string opts = "/" + std::to_string(a.prefix);
    if (a.type == InetAddrType::IPV6) {
        return toString(a.getAddr<in6_addr>()) + opts;
    }
    if (a.type == InetAddrType::IPV4) {
        return toString(a.getAddr<in_addr>()) + opts;
    }
    return "";
}

InetAddr::InetAddr(const std::string& a, unsigned p, uint32_t f) :
    prefix(p),
    flags(f)
{
    if (a.find(":") != std::string::npos) {
        type = InetAddrType::IPV6;
        fromString(a, getAddr<in6_addr>());
    } else {
        type = InetAddrType::IPV4;
        fromString(a, getAddr<in_addr>());
    }
}

void NetworkInterface::create(InterfaceType type,
                              const std::string& peerif,
                              MacVLanMode mode)
{
    switch (type) {
        case InterfaceType::VETH:
            createVeth(peerif);
            break;
        case InterfaceType::BRIDGE:
            createBridge();
            break;
        case InterfaceType::MACVLAN:
            createMacVLan(peerif, mode);
            break;
        default:
            throw NetworkException("Unsuported interface type");
    }
}

void NetworkInterface::createVeth(const std::string& peerif)
{
    NetlinkMessage nlm(RTM_NEWLINK, NLM_F_REQUEST | NLM_F_CREATE | NLM_F_EXCL | NLM_F_ACK);
    ifinfomsg info = utils::make_clean<ifinfomsg>();
    info.ifi_family = AF_UNSPEC;
    info.ifi_change = CHANGE_FLAGS_DEFAULT;
    nlm.put(info)
        .put(IFLA_IFNAME, mIfname)
        .beginNested(IFLA_LINKINFO)
            .put(IFLA_INFO_KIND, "veth")
            .beginNested(IFLA_INFO_DATA)
                .beginNested(VETH_INFO_PEER)
                    .put(info)
                    .put(IFLA_IFNAME, peerif)
                .endNested()
            .endNested()
        .endNested();
    send(nlm, mContainerPid);
}

void NetworkInterface::createBridge()
{
    NetlinkMessage nlm(RTM_NEWLINK, NLM_F_REQUEST | NLM_F_CREATE | NLM_F_EXCL | NLM_F_ACK);
    ifinfomsg info = utils::make_clean<ifinfomsg>();
    info.ifi_family = AF_UNSPEC;
    info.ifi_change = CHANGE_FLAGS_DEFAULT;
    nlm.put(info)
        .beginNested(IFLA_LINKINFO)
            .put(IFLA_INFO_KIND, "bridge")
            .beginNested(IFLA_INFO_DATA)
                .beginNested(IFLA_AF_SPEC)
                    .put<uint32_t>(IFLA_BRIDGE_FLAGS, BRIDGE_FLAGS_MASTER)
                .endNested()
            .endNested()
        .endNested()
        .put(IFLA_IFNAME, mIfname); //bridge name (will be created)
    send(nlm, mContainerPid);
}

void NetworkInterface::createMacVLan(const std::string& maserif, MacVLanMode mode)
{
    uint32_t index = getInterfaceIndex(maserif);
    NetlinkMessage nlm(RTM_NEWLINK, NLM_F_REQUEST | NLM_F_CREATE | NLM_F_EXCL | NLM_F_ACK);
    ifinfomsg info = utils::make_clean<ifinfomsg>();
    info.ifi_family = AF_UNSPEC;
    info.ifi_change = CHANGE_FLAGS_DEFAULT;
    nlm.put(info)
        .beginNested(IFLA_LINKINFO)
            .put(IFLA_INFO_KIND, "macvlan")
            .beginNested(IFLA_INFO_DATA)
                .put(IFLA_MACVLAN_MODE, static_cast<uint32_t>(mode))
            .endNested()
        .endNested()
        .put(IFLA_LINK, index)      //master index
        .put(IFLA_IFNAME, mIfname); //slave name (will be created)
    send(nlm, mContainerPid);
}

void NetworkInterface::moveToContainer(pid_t pid)
{
    NetlinkMessage nlm(RTM_NEWLINK, NLM_F_REQUEST | NLM_F_ACK);
    ifinfomsg info = utils::make_clean<ifinfomsg>();
    info.ifi_family = AF_UNSPEC;
    info.ifi_index = getInterfaceIndex(mIfname);
    nlm.put(info)
        .put(IFLA_NET_NS_PID, pid);
    send(nlm, mContainerPid);
    mContainerPid = pid;
}

void NetworkInterface::destroy()
{
    //uint32_t index = getInterfaceIndex(mIfname);
    NetlinkMessage nlm(RTM_DELLINK, NLM_F_REQUEST | NLM_F_ACK);
    ifinfomsg info = utils::make_clean<ifinfomsg>();
    info.ifi_family = AF_UNSPEC;
    info.ifi_change = CHANGE_FLAGS_DEFAULT;
    info.ifi_index = getInterfaceIndex(mContainerPid, mIfname);
    nlm.put(info)
        .put(IFLA_IFNAME, mIfname);
    send(nlm, mContainerPid);
}

NetStatus NetworkInterface::status() const
{
    const Attrs& attrs = getAttrs();
    NetStatus st = NetStatus::DOWN;
    for (const Attr& a : attrs)
        if (a.name == AttrName::FLAGS) {
            uint32_t f = stoul(a.value);
            if (f & IFF_UP) {
                st = NetStatus::UP;
                break;
            }
        }
    return st;
}

void NetworkInterface::renameFrom(const std::string& oldif)
{
    NetlinkMessage nlm(RTM_SETLINK, NLM_F_REQUEST | NLM_F_ACK);
    ifinfomsg info = utils::make_clean<ifinfomsg>();
    info.ifi_family = AF_UNSPEC;
    info.ifi_index = getInterfaceIndex(mContainerPid, oldif);
    info.ifi_change = CHANGE_FLAGS_DEFAULT;

    nlm.put(info)
        .put(IFLA_IFNAME, mIfname);
    send(nlm, mContainerPid);
}

void NetworkInterface::addToBridge(const std::string& bridge)
{
    bridgeModify(mContainerPid, mIfname, getInterfaceIndex(mContainerPid, bridge));
}

void NetworkInterface::delFromBridge()
{
    bridgeModify(mContainerPid, mIfname, 0);
}

void NetworkInterface::setAttrs(const Attrs& attrs)
{
    if (attrs.empty()) {
        return ;
    }

    //TODO check this: NetlinkMessage nlm(RTM_SETLINK, NLM_F_REQUEST | NLM_F_ACK);
    NetlinkMessage nlm(RTM_NEWLINK, NLM_F_REQUEST | NLM_F_CREATE | NLM_F_ACK);
    ifinfomsg info = utils::make_clean<ifinfomsg>();
    info.ifi_index = getInterfaceIndex(mContainerPid, mIfname);
    info.ifi_family = AF_UNSPEC;
    info.ifi_change = CHANGE_FLAGS_DEFAULT;

    std::string mac;
    unsigned mtu = 0, link = 0, txq = 0;
    for (const auto& attr : attrs) {
        if (attr.name == AttrName::FLAGS) {
            info.ifi_flags = stoul(attr.value);
        } else if (attr.name == AttrName::CHANGE) {
            info.ifi_change = stoul(attr.value);
        } else if (attr.name == AttrName::TYPE) {
            info.ifi_type = stoul(attr.value);
        } else if (attr.name == AttrName::MTU) {
            mtu = stoul(attr.value);
        } else if (attr.name == AttrName::LINK) {
            link = stoul(attr.value);
        } else if (attr.name == AttrName::TXQLEN) {
            txq = stoul(attr.value);
        } else if (attr.name == AttrName::MAC) {
            mac = attr.value;
        }

    }
    nlm.put(info);
    if (mtu) {
        nlm.put<uint32_t>(IFLA_MTU, mtu);
    }
    if (link) {
        nlm.put<uint32_t>(IFLA_LINK, link);
    }
    if (txq) {
        nlm.put<uint32_t>(IFLA_TXQLEN, txq);
    }
    if (!mac.empty()) {
        char buf[6];
        toMacAddressArray(mac, buf, 6);
        nlm.put(IFLA_ADDRESS, buf);
    }

    NetlinkResponse response = send(nlm, mContainerPid);
    if (!response.hasMessage()) {
        throw NetworkException("Can't set interface information");
    }
}

Attrs NetworkInterface::getAttrs() const
{
    NetlinkMessage nlm(RTM_GETLINK, NLM_F_REQUEST | NLM_F_ACK);
    ifinfomsg info = utils::make_clean<ifinfomsg>();
    info.ifi_family = AF_UNSPEC;
    info.ifi_change = CHANGE_FLAGS_DEFAULT;
    nlm.put(info)
        .put(IFLA_IFNAME, mIfname);

    NetlinkResponse response = send(nlm, mContainerPid);
    if (!response.hasMessage()) {
        throw NetworkException("Can't get interface information");
    }

    response.fetch(info);
    Attrs attrs;
    attrs.push_back(Attr{AttrName::FLAGS, std::to_string(info.ifi_flags)});
    attrs.push_back(Attr{AttrName::TYPE, std::to_string(info.ifi_type)});

    while (response.hasAttribute()) {
        /*
         * While traditional MAC addresses are all 48 bits in length,
         * a few types of networks require 64-bit addresses instead.
         */
        std::string mac;
        uint32_t tmp;
        int attrType = response.getAttributeType();
        switch (attrType) {
        case IFLA_ADDRESS: //1
            response.fetch(IFLA_ADDRESS, mac);
            attrs.push_back(Attr{AttrName::MAC, utils::toHexString(mac.c_str(), mac.size())});
            break;
        case IFLA_MTU: //4
            response.fetch(IFLA_MTU, tmp);
            attrs.push_back(Attr{AttrName::MTU, std::to_string(tmp)});
            break;
        case IFLA_LINK://5
            response.fetch(IFLA_LINK, tmp);
            attrs.push_back(Attr{AttrName::LINK, std::to_string(tmp)});
            break;
        case IFLA_TXQLEN://13
            response.fetch(IFLA_TXQLEN, tmp);
            attrs.push_back(Attr{AttrName::TXQLEN, std::to_string(tmp)});
            break;
        case IFLA_OPERSTATE://16  (IF_OPER_DOWN,...,IF_OPER_UP)
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
        case IFLA_WEIGHT:   //15
        default:
            response.skipAttribute();
            break;
        }
    }
    return attrs;
}

void NetworkInterface::addInetAddr(const InetAddr& addr)
{
    NetlinkMessage nlm(RTM_NEWADDR, NLM_F_CREATE | NLM_F_REQUEST | NLM_F_ACK);
    ifaddrmsg infoAddr = utils::make_clean<ifaddrmsg>();
    infoAddr.ifa_index = getInterfaceIndex(mContainerPid, mIfname);
    infoAddr.ifa_family = addr.type == InetAddrType::IPV4 ? AF_INET : AF_INET6;
    infoAddr.ifa_prefixlen = addr.prefix;
    infoAddr.ifa_flags = addr.flags;
    nlm.put(infoAddr);

    if (addr.type == InetAddrType::IPV6) {
        nlm.put(IFA_ADDRESS, addr.getAddr<in6_addr>());
        nlm.put(IFA_LOCAL, addr.getAddr<in6_addr>());
    } else if (addr.type == InetAddrType::IPV4) {
        nlm.put(IFA_ADDRESS, addr.getAddr<in_addr>());
        nlm.put(IFA_LOCAL, addr.getAddr<in_addr>());
    }

    send(nlm, mContainerPid);
}

void NetworkInterface::delInetAddr(const InetAddr& addr)
{
    NetlinkMessage nlm(RTM_DELADDR, NLM_F_REQUEST | NLM_F_ACK);
    ifaddrmsg infoAddr = utils::make_clean<ifaddrmsg>();
    infoAddr.ifa_index = getInterfaceIndex(mContainerPid, mIfname);
    infoAddr.ifa_family = addr.type == InetAddrType::IPV4 ? AF_INET : AF_INET6;
    infoAddr.ifa_prefixlen = addr.prefix;
    infoAddr.ifa_flags = addr.flags;
    nlm.put(infoAddr);

    if (addr.type == InetAddrType::IPV6) {
        nlm.put(IFA_ADDRESS, addr.getAddr<in6_addr>());
        nlm.put(IFA_LOCAL, addr.getAddr<in6_addr>());
    } else if (addr.type == InetAddrType::IPV4) {
        nlm.put(IFA_ADDRESS, addr.getAddr<in_addr>());
        nlm.put(IFA_LOCAL, addr.getAddr<in_addr>());
    }

    send(nlm, mContainerPid);
}

std::vector<InetAddr> NetworkInterface::getInetAddressList() const
{
    std::vector<InetAddr> addrs;
    getAddressList(mContainerPid, addrs, AF_UNSPEC, mIfname);
    return addrs;
}

static rt_class_t getRoutingTableClass(const RoutingTable rt)
{
    switch (rt) {
    case RoutingTable::UNSPEC:
        return RT_TABLE_UNSPEC; //0
    case RoutingTable::COMPAT:
        return RT_TABLE_COMPAT; //252
    case RoutingTable::DEFAULT:
        return RT_TABLE_DEFAULT;//253
    case RoutingTable::MAIN:
        return RT_TABLE_MAIN;   //254
    case RoutingTable::LOCAL:
        return RT_TABLE_LOCAL;  //255
    default: //all other are user tables (1...251), but I use only '1'
        return static_cast<rt_class_t>(1);
    }
}
static RoutingTable getRoutingTable(unsigned tbl)
{
    switch (tbl) {
    case RT_TABLE_UNSPEC:
        return RoutingTable::UNSPEC;
    case RT_TABLE_COMPAT:
        return RoutingTable::COMPAT;
    case RT_TABLE_DEFAULT:
        return RoutingTable::DEFAULT;
    case RT_TABLE_MAIN:
        return RoutingTable::MAIN;
    case RT_TABLE_LOCAL:
        return RoutingTable::LOCAL;
    default:
        return RoutingTable::USER;
    }
}

void NetworkInterface::addRoute(const Route& route, const RoutingTable rt)
{
    InetAddrType type = route.dst.type;
    if (route.src.type != type) {
        const std::string msg = "Family type must be the same";
        LOGE(msg);
        throw NetworkException(msg);
    }
    uint32_t index = getInterfaceIndex(mContainerPid, mIfname);
    NetlinkMessage nlm(RTM_NEWROUTE, NLM_F_REQUEST | NLM_F_CREATE | NLM_F_EXCL | NLM_F_ACK);

    rtmsg msg = utils::make_clean<rtmsg>();

    if (type == InetAddrType::IPV6) {
        msg.rtm_family = AF_INET6;
    } else if (type == InetAddrType::IPV4) {
        msg.rtm_family = AF_INET;
    }
    msg.rtm_table = getRoutingTableClass(rt);
    msg.rtm_protocol = RTPROT_BOOT;
    msg.rtm_scope = RT_SCOPE_UNIVERSE;
    msg.rtm_type = RTN_UNICAST;
    msg.rtm_dst_len = route.dst.prefix;

    nlm.put(msg);

    if (type == InetAddrType::IPV6) {
        if (route.dst.prefix == 0)
            nlm.put(RTA_GATEWAY, route.dst.getAddr<in6_addr>());
        else
            nlm.put(RTA_DST, route.dst.getAddr<in6_addr>());

        if (route.src.prefix == 128) {
            nlm.put(RTA_PREFSRC, route.src.getAddr<in6_addr>());
        }
    } else if (type == InetAddrType::IPV4) {
        if (route.dst.prefix == 0)
            nlm.put(RTA_GATEWAY, route.dst.getAddr<in_addr>());
        else
            nlm.put(RTA_DST, route.dst.getAddr<in_addr>());

        if (route.src.prefix == 32) {
            nlm.put(RTA_PREFSRC, route.src.getAddr<in_addr>());
        }
    }

    nlm.put(RTA_OIF, index);

    send(nlm, mContainerPid);
}

void NetworkInterface::delRoute(const Route& route, const RoutingTable rt)
{
    InetAddrType type = route.dst.type;
    if (route.src.type != type) {
        const std::string msg = "Family type must be the same";
        LOGE(msg);
        throw NetworkException(msg);
    }
    uint32_t index = getInterfaceIndex(mContainerPid, mIfname);
    NetlinkMessage nlm(RTM_DELROUTE, NLM_F_REQUEST | NLM_F_ACK);

    rtmsg msg = utils::make_clean<rtmsg>();
    msg.rtm_scope = RT_SCOPE_NOWHERE;
    msg.rtm_table = getRoutingTableClass(rt);
    msg.rtm_dst_len = route.dst.prefix;
    if (type == InetAddrType::IPV6) {
        msg.rtm_family = AF_INET6;
    } else if (type == InetAddrType::IPV4) {
        msg.rtm_family = AF_INET;
    }
    nlm.put(msg);

    if (type == InetAddrType::IPV6) {
        nlm.put(RTA_DST, route.dst.getAddr<in6_addr>());
    } else if (type == InetAddrType::IPV4) {
        nlm.put(RTA_DST, route.dst.getAddr<in_addr>());
    }
    nlm.put(RTA_OIF, index);

    send(nlm, mContainerPid);
}

static std::vector<Route> getRoutesImpl(pid_t pid, rt_class_t tbl, const std::string& ifname, int family)
{
    uint32_t searchindex = 0;
    NetlinkMessage nlm(RTM_GETROUTE, NLM_F_REQUEST | NLM_F_ACK | NLM_F_DUMP);
    ifinfomsg info = utils::make_clean<ifinfomsg>();
    info.ifi_family = family;
    info.ifi_change = CHANGE_FLAGS_DEFAULT;

    nlm.put(info);

    NetlinkResponse response = send(nlm, pid);

    if (!ifname.empty()) {
        searchindex = getInterfaceIndex(pid, ifname);
    }

    std::vector<Route> routes;
    for ( ; response.hasMessage(); response.fetchNextMessage()) {
        if (response.getMessageType() != RTM_NEWROUTE) {
            LOGW("Not route info in response");
            continue;
        }
        rtmsg rt;
        response.fetch(rt);
        if (tbl != RT_TABLE_UNSPEC && rt.rtm_table != tbl) {
            continue;
        }
        if (rt.rtm_flags & RTM_F_CLONED) {
            continue;
        }

        Route route;
        uint32_t index = 0;
        route.dst.flags = 0;
        route.src.flags = 0;
        route.dst.prefix = 0;
        route.src.prefix = 0;
        route.table = getRoutingTable(rt.rtm_table);

        if (rt.rtm_family == AF_INET6) {
            route.dst.type = InetAddrType::IPV6;
            route.src.type = InetAddrType::IPV6;
        } else if (rt.rtm_family == AF_INET) {
            route.dst.type = InetAddrType::IPV4;
            route.src.type = InetAddrType::IPV4;
        } else {
            const std::string msg = "Unsupported inet family";
            LOGE(msg);
            throw NetworkException(msg);
        }

        route.dst.prefix = rt.rtm_dst_len;
        while (response.hasAttribute()) {
            std::string tmpval;
            uint32_t tmp;
            int attrType = response.getAttributeType();
            //int len = response.getAttributeLength();
            switch (attrType) {
            case RTA_DST:    // 1
            case RTA_GATEWAY:// 5
                if (route.dst.type == InetAddrType::IPV6) {
                    response.fetch(attrType, route.dst.getAddr<in6_addr>());
                } else {
                    response.fetch(attrType, route.dst.getAddr<in_addr>());
                }
                break;
            case RTA_SRC:    // 2
            case RTA_PREFSRC:// 7
                if (route.src.type == InetAddrType::IPV6) {
                    response.fetch(attrType, route.src.getAddr<in6_addr>());
                    route.src.prefix = 128;
                } else {
                    response.fetch(attrType, route.src.getAddr<in_addr>());
                    route.src.prefix = 32;
                }
                break;
            case RTA_OIF: //4
                response.fetch(RTA_OIF, tmp);
                index = tmp;
                break;
            case RTA_PRIORITY: // 6
                response.fetch(RTA_PRIORITY, tmp);
                route.metric = tmp;
                break;
            case RTA_TABLE:    //15  extends and ovewrites rt.rtm_table
                response.fetch(RTA_TABLE, tmp);
                route.table = getRoutingTable(tmp);
                break;

            case RTA_CACHEINFO://12
                response.skipAttribute();
                break;
            case RTA_IIF:      // 3
            case RTA_METRICS:  // 8  array of struct rtattr
            case RTA_MULTIPATH:// 9  array of struct rtnexthop
            case RTA_FLOW:     //11
            default:
                if (!searchindex) {
                    response.fetch(attrType, tmpval);
                    LOGW("rtAttr " << attrType << ":" <<
                        utils::toHexString(tmpval.c_str(), tmpval.size()));
                } else {
                    response.skipAttribute();
                }
                break;
            }
        }
        if (index == 0) continue;

        if (searchindex == 0 || searchindex == index) {
            route.ifname = getInterfaceName(index);
            routes.push_back(route);
        }
    }
    return routes;
}

std::vector<Route> NetworkInterface::getRoutes(const RoutingTable rt) const
{
    return getRoutesImpl(mContainerPid, getRoutingTableClass(rt), mIfname, AF_UNSPEC);
}

void NetworkInterface::up()
{
    Attrs attrs;
    attrs.push_back(Attr{AttrName::CHANGE, std::to_string(IFF_UP)});
    attrs.push_back(Attr{AttrName::FLAGS, std::to_string(IFF_UP)});
    setAttrs(attrs);
}

void NetworkInterface::down()
{
    Attrs attrs;
    attrs.push_back(Attr{AttrName::CHANGE, std::to_string(IFF_UP)});
    attrs.push_back(Attr{AttrName::FLAGS, std::to_string(0)});
    setAttrs(attrs);
}

void NetworkInterface::setMACAddress(const std::string& macaddr)
{
    Attrs attrs;
    attrs.push_back(Attr{AttrName::MAC, macaddr});
    setAttrs(attrs);
}

void NetworkInterface::setMTU(int mtu)
{
    Attrs attrs;
    attrs.push_back(Attr{AttrName::MTU, std::to_string(mtu)});
    setAttrs(attrs);
}

void NetworkInterface::setTxLength(int txqlen)
{
    Attrs attrs;
    attrs.push_back(Attr{AttrName::TXQLEN, std::to_string(txqlen)});
    setAttrs(attrs);
}


std::vector<std::string> NetworkInterface::getInterfaces(pid_t initpid)
{
    // get interfaces seen by netlink
    NetlinkMessage nlm(RTM_GETLINK, NLM_F_REQUEST | NLM_F_DUMP | NLM_F_ROOT);
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

std::vector<Route> NetworkInterface::getRoutes(pid_t initpid, const RoutingTable rt)
{
    rt_class_t tbl = getRoutingTableClass(rt);
    return getRoutesImpl(initpid, tbl, "", AF_UNSPEC);
}

} // namespace lxcpp
