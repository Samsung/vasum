/*
 *  Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Mateusz Malicki <m.malicki2@samsung.com>
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
 * @author  Mateusz Malicki (m.malicki2@samsung.com)
 * @brief   Network devices management functions definition
 */

#include "config.hpp"
#include "netdev.hpp"
#include "netlink/netlink-message.hpp"
#include "utils/make-clean.hpp"
#include "utils/exception.hpp"
#include "utils.hpp"
#include "exception.hpp"
#include "logger/logger.hpp"

#include <algorithm>
#include <string>
#include <cstdint>
#include <cstring>
#include <cassert>
#include <sstream>
#include <set>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

#include <net/if.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <linux/rtnetlink.h>
#include <linux/veth.h>
#include <linux/sockios.h>
#include <linux/if_link.h>
#include <linux/rtnetlink.h>
#include <linux/if_bridge.h>

//IFLA_BRIDGE_FLAGS and BRIDGE_FLAGS_MASTER
//should be defined in linux/if_bridge.h since kernel v3.7
#ifndef IFLA_BRIDGE_FLAGS
#define IFLA_BRIDGE_FLAGS 0
#endif
#ifndef BRIDGE_FLAGS_MASTER
#define BRIDGE_FLAGS_MASTER 1
#endif

using namespace std;
using namespace utils;
using namespace vasum::netlink;

namespace vasum {
namespace netdev {

namespace {

string getUniqueVethName()
{
    auto find = [](const ifaddrs* ifaddr, const string& name) -> bool {
        for (const ifaddrs* ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
            if (name == ifa->ifa_name) {
                return true;
            }
        }
        return false;
    };

    ifaddrs* ifaddr;
    getifaddrs(&ifaddr);
    string newName;
    int i = 0;
    do {
        newName = "veth0" + to_string(++i);
    } while (find(ifaddr, newName));

    freeifaddrs(ifaddr);
    return newName;
}

uint32_t getInterfaceIndex(const string& name) {
    uint32_t index = if_nametoindex(name.c_str());
    if (!index) {
        const std::string msg = "Can't get " + name + " interface index (" + getSystemErrorMessage() + ")";
        LOGE(msg);
        throw ZoneOperationException(msg);
    }
    return index;
}

uint32_t getInterfaceIndex(const string& name, pid_t nsPid) {
    NetlinkMessage nlm(RTM_GETLINK, NLM_F_REQUEST | NLM_F_ACK);
    ifinfomsg infoPeer = utils::make_clean<ifinfomsg>();
    infoPeer.ifi_family = AF_UNSPEC;
    infoPeer.ifi_change = 0xFFFFFFFF;
    nlm.put(infoPeer)
        .put(IFLA_IFNAME, name);
    NetlinkResponse response = send(nlm, nsPid);
    if (!response.hasMessage()) {
        throw VasumException("Can't get interface index");
    }

    response.fetch(infoPeer);
    return infoPeer.ifi_index;
}

int getIpFamily(const std::string& ip)
{
    return ip.find(':') == std::string::npos ? AF_INET : AF_INET6;
}

void validateNetdevName(const string& name)
{
    if (name.size() <= 1 || name.size() >= IFNAMSIZ) {
        throw ZoneOperationException("Invalid netdev name format");
    }
}

void createPipedNetdev(const string& netdev1, const string& netdev2)
{
    validateNetdevName(netdev1);
    validateNetdevName(netdev2);

    NetlinkMessage nlm(RTM_NEWLINK, NLM_F_REQUEST | NLM_F_CREATE | NLM_F_EXCL | NLM_F_ACK);
    ifinfomsg infoPeer = utils::make_clean<ifinfomsg>();
    infoPeer.ifi_family = AF_UNSPEC;
    infoPeer.ifi_change = 0xFFFFFFFF;
    nlm.put(infoPeer)
        .beginNested(IFLA_LINKINFO)
            .put(IFLA_INFO_KIND, "veth")
            .beginNested(IFLA_INFO_DATA)
                .beginNested(VETH_INFO_PEER)
                    .put(infoPeer)
                    .put(IFLA_IFNAME, netdev2)
                .endNested()
            .endNested()
        .endNested()
        .put(IFLA_IFNAME, netdev1);
    send(nlm);
}

void attachToBridge(const string& bridge, const string& netdev)
{
    validateNetdevName(bridge);
    validateNetdevName(netdev);

    uint32_t index = getInterfaceIndex(netdev);
    int fd = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (fd < 0) {
        const std::string msg = "Can't open socket (" + getSystemErrorMessage() + ")";
        LOGE(msg);
        throw ZoneOperationException(msg);
    }

    struct ifreq ifr = utils::make_clean<ifreq>();
    strncpy(ifr.ifr_name, bridge.c_str(), IFNAMSIZ);
    ifr.ifr_ifindex = index;
    int err = ioctl(fd, SIOCBRADDIF, &ifr);
    if (err < 0) {
        int error = errno;
        //TODO: Close can be interrupted. Move util functions from ipc
        ::close(fd);
        const std::string msg = "Can't attach to bridge (" + getSystemErrorMessage(error) + ")";
        LOGE(msg);
        throw ZoneOperationException(msg);
    }
    close(fd);
}

int setFlags(const string& name, uint32_t mask, uint32_t flags)
{
    uint32_t index = getInterfaceIndex(name);
    NetlinkMessage nlm(RTM_NEWLINK, NLM_F_REQUEST | NLM_F_ACK);
    ifinfomsg infoPeer = utils::make_clean<ifinfomsg>();
    infoPeer.ifi_family = AF_UNSPEC;
    infoPeer.ifi_index = index;
    infoPeer.ifi_flags = flags;
    // since kernel v2.6.22 ifi_change is used to change only selected flags;
    infoPeer.ifi_change = mask;
    nlm.put(infoPeer);
    send(nlm);
    return 0;
}

void up(const string& netdev)
{
    setFlags(netdev, IFF_UP, IFF_UP);
}

void moveToNS(const string& netdev, pid_t pid)
{
    uint32_t index = getInterfaceIndex(netdev);
    NetlinkMessage nlm(RTM_NEWLINK, NLM_F_REQUEST | NLM_F_ACK);
    ifinfomsg infopeer = utils::make_clean<ifinfomsg>();
    infopeer.ifi_family = AF_UNSPEC;
    infopeer.ifi_index = index;
    nlm.put(infopeer)
        .put(IFLA_NET_NS_PID, pid);
    send(nlm);
}

void createMacvlan(const string& master, const string& slave, const macvlan_mode& mode)
{
    validateNetdevName(master);
    validateNetdevName(slave);

    uint32_t index = getInterfaceIndex(master);
    NetlinkMessage nlm(RTM_NEWLINK, NLM_F_REQUEST|NLM_F_CREATE|NLM_F_EXCL|NLM_F_ACK);
    ifinfomsg infopeer = utils::make_clean<ifinfomsg>();
    infopeer.ifi_family = AF_UNSPEC;
    infopeer.ifi_change = 0xFFFFFFFF;
    nlm.put(infopeer)
        .beginNested(IFLA_LINKINFO)
            .put(IFLA_INFO_KIND, "macvlan")
            .beginNested(IFLA_INFO_DATA)
                .put(IFLA_MACVLAN_MODE, static_cast<uint32_t>(mode))
            .endNested()
        .endNested()
        .put(IFLA_LINK, index)
        .put(IFLA_IFNAME, slave);
    send(nlm);
}

std::vector<Attrs> getIpAddresses(const pid_t nsPid, int family, uint32_t index)
{
    NetlinkMessage nlm(RTM_GETADDR, NLM_F_REQUEST | NLM_F_ACK | NLM_F_DUMP);
    ifaddrmsg infoAddr = utils::make_clean<ifaddrmsg>();
    infoAddr.ifa_family = family;
    nlm.put(infoAddr);
    NetlinkResponse response = send(nlm, nsPid);
    if (!response.hasMessage()) {
        //There is no interfaces with addresses
        return std::vector<Attrs>();
    }

    std::vector<Attrs> addresses;
    while (response.hasMessage()) {
        ifaddrmsg addrmsg;
        response.fetch(addrmsg);
        if (addrmsg.ifa_index == index) {
            Attrs attrs;
            attrs.push_back(make_tuple("prefixlen", std::to_string(addrmsg.ifa_prefixlen)));
            attrs.push_back(make_tuple("flags", std::to_string(addrmsg.ifa_flags)));
            attrs.push_back(make_tuple("scope", std::to_string(addrmsg.ifa_scope)));
            attrs.push_back(make_tuple("family", std::to_string(addrmsg.ifa_family)));
            while (response.hasAttribute()) {
                assert(INET6_ADDRSTRLEN >= INET_ADDRSTRLEN);
                char buf[INET6_ADDRSTRLEN];
                in6_addr addr6;
                in_addr addr4;
                const void* addr = NULL;
                int attrType = response.getAttributeType();
                switch (attrType) {
                    case IFA_ADDRESS:
                        if (family == AF_INET6) {
                            response.fetch(IFA_ADDRESS, addr6);
                            addr = &addr6;
                        } else {
                            assert(family == AF_INET);
                            response.fetch(IFA_ADDRESS, addr4);
                            addr = &addr4;
                        }
                        addr = inet_ntop(family, addr, buf, sizeof(buf));
                        if (addr == NULL) {
                            const std::string msg = "Can't convert ip address: " + getSystemErrorMessage();
                            LOGE(msg);
                            throw VasumException(msg);
                        }
                        attrs.push_back(make_tuple("ip", buf));
                        break;
                    default:
                        response.skipAttribute();
                        break;
                }
            }
            addresses.push_back(std::move(attrs));
        }
        response.fetchNextMessage();
    }
    return addresses;
}

void  setIpAddresses(const pid_t nsPid,
                     const uint32_t index,
                     const Attrs& attrs,
                     int family)
{
    NetlinkMessage nlm(RTM_NEWADDR, NLM_F_CREATE | NLM_F_REQUEST | NLM_F_ACK);
    ifaddrmsg infoAddr = utils::make_clean<ifaddrmsg>();
    infoAddr.ifa_family = family;
    infoAddr.ifa_index = index;
    for (const auto& attr : attrs) {
        if (get<0>(attr) == "prefixlen") {
            infoAddr.ifa_prefixlen = stoul(get<1>(attr));
        }
        if (get<0>(attr) == "flags") {
            infoAddr.ifa_flags = stoul(get<1>(attr));
        }
        if (get<0>(attr) == "scope") {
            infoAddr.ifa_scope = stoul(get<1>(attr));
        }
    }
    nlm.put(infoAddr);
    for (const auto& attr : attrs) {
        if (get<0>(attr) == "ip") {
            if (family == AF_INET6) {
                in6_addr addr6;
                if (inet_pton(AF_INET6, get<1>(attr).c_str(), &addr6) != 1) {
                    throw VasumException("Can't set ipv4 address");
                };
                nlm.put(IFA_ADDRESS, addr6);
                nlm.put(IFA_LOCAL, addr6);
            } else {
                assert(family == AF_INET);
                in_addr addr4;
                if (inet_pton(AF_INET, get<1>(attr).c_str(), &addr4) != 1) {
                    throw VasumException("Can't set ipv6 address");
                };
                nlm.put(IFA_LOCAL, addr4);
            }
        }
    }
    send(nlm, nsPid);
}

void deleteIpAddress(const pid_t nsPid,
                     const uint32_t index,
                     const std::string& ip,
                     int prefixlen,
                     int family)
{
    NetlinkMessage nlm(RTM_DELADDR, NLM_F_REQUEST | NLM_F_ACK);
    ifaddrmsg infoAddr = utils::make_clean<ifaddrmsg>();
    infoAddr.ifa_family = family;
    infoAddr.ifa_index = index;
    infoAddr.ifa_prefixlen = prefixlen;
    nlm.put(infoAddr);
    if (family == AF_INET6) {
        in6_addr addr6;
        if (inet_pton(AF_INET6, ip.c_str(), &addr6) != 1) {
            throw VasumException("Can't delete ipv6 address");
        };
        nlm.put(IFA_ADDRESS, addr6);
        nlm.put(IFA_LOCAL, addr6);
    } else {
        assert(family == AF_INET);
        in_addr addr4;
        if (inet_pton(AF_INET, ip.c_str(), &addr4) != 1) {
            throw VasumException("Can't delete ipv4 address");
        };
        nlm.put(IFA_LOCAL, addr4);
    }
    send(nlm, nsPid);
}

} // namespace

void createVeth(const pid_t& nsPid, const string& nsDev, const string& hostDev)
{
    string hostVeth = getUniqueVethName();
    LOGT("Creating veth: bridge: " << hostDev << ", port: " << hostVeth << ", zone: " << nsDev);
    createPipedNetdev(nsDev, hostVeth);
    try {
        attachToBridge(hostDev, hostVeth);
        up(hostVeth);
        moveToNS(nsDev, nsPid);
    } catch(const exception& ex) {
        try {
            destroyNetdev(hostVeth);
        } catch (const exception& ex) {
            LOGE("Can't destroy netdev pipe: " << hostVeth << ", " << nsDev);
        }
        throw;
    }
}

void createMacvlan(const pid_t& nsPid,
                   const string& nsDev,
                   const string& hostDev,
                   const macvlan_mode& mode)
{
    LOGT("Creating macvlan: host: " << hostDev << ", zone: " << nsDev << ", mode: " << mode);
    createMacvlan(hostDev, nsDev, mode);
    try {
        up(nsDev);
        moveToNS(nsDev, nsPid);
    } catch(const exception& ex) {
        try {
            destroyNetdev(nsDev);
        } catch (const exception& ex) {
            LOGE("Can't destroy netdev: " << nsDev);
        }
        throw;
    }
}

void movePhys(const pid_t& nsPid, const string& devId)
{
    LOGT("Creating phys: dev: " << devId);
    moveToNS(devId, nsPid);
}

std::vector<std::string> listNetdev(const pid_t& nsPid)
{
    NetlinkMessage nlm(RTM_GETLINK, NLM_F_REQUEST|NLM_F_DUMP|NLM_F_ROOT);
    ifinfomsg info = utils::make_clean<ifinfomsg>();
    info.ifi_family = AF_PACKET;
    nlm.put(info);
    NetlinkResponse response = send(nlm, nsPid);
    std::vector<std::string> interfaces;
    while (response.hasMessage()) {
        std::string ifName;
        response.skip<ifinfomsg>();
        response.fetch(IFLA_IFNAME, ifName);
        interfaces.push_back(ifName);
        response.fetchNextMessage();
    }
    return interfaces;
}

void destroyNetdev(const string& netdev, const pid_t pid)
{
    LOGT("Destroying netdev: " << netdev);
    validateNetdevName(netdev);

    NetlinkMessage nlm(RTM_DELLINK, NLM_F_REQUEST|NLM_F_ACK);
    ifinfomsg infopeer = utils::make_clean<ifinfomsg>();
    infopeer.ifi_family = AF_UNSPEC;
    infopeer.ifi_change = 0xFFFFFFFF;
    nlm.put(infopeer)
        .put(IFLA_IFNAME, netdev);
    send(nlm, pid);
}

void createBridge(const string& netdev)
{
    LOGT("Creating bridge: " << netdev);
    validateNetdevName(netdev);

    NetlinkMessage nlm(RTM_NEWLINK, NLM_F_REQUEST | NLM_F_CREATE | NLM_F_EXCL | NLM_F_ACK);
    ifinfomsg infoPeer = utils::make_clean<ifinfomsg>();
    infoPeer.ifi_family = AF_UNSPEC;
    infoPeer.ifi_change = 0xFFFFFFFF;
    nlm.put(infoPeer)
        .beginNested(IFLA_LINKINFO)
            .put(IFLA_INFO_KIND, "bridge")
            .beginNested(IFLA_INFO_DATA)
                .beginNested(IFLA_AF_SPEC)
                    .put<uint32_t>(IFLA_BRIDGE_FLAGS, BRIDGE_FLAGS_MASTER)
                .endNested()
            .endNested()
        .endNested()
        .put(IFLA_IFNAME, netdev);
    send(nlm);
}

Attrs getAttrs(const pid_t nsPid, const std::string& netdev)
{
    auto joinAddresses = [](const Attrs& attrs) -> std::string {
        bool first = true;
        stringstream ss;
        for (const auto& attr : attrs) {
            ss << (first ? "" : ",") << get<0>(attr) << ":" << get<1>(attr);
            first = false;
        }
        return ss.str();
    };

    LOGT("Getting network device informations: " << netdev);
    validateNetdevName(netdev);

    NetlinkMessage nlm(RTM_GETLINK, NLM_F_REQUEST | NLM_F_ACK);
    ifinfomsg infoPeer = utils::make_clean<ifinfomsg>();
    infoPeer.ifi_family = AF_UNSPEC;
    infoPeer.ifi_change = 0xFFFFFFFF;
    nlm.put(infoPeer)
        .put(IFLA_IFNAME, netdev);
    Attrs attrs;
    try {
        NetlinkResponse response = send(nlm, nsPid);
        if (!response.hasMessage()) {
            throw VasumException("Can't get interface information");
        }
        response.fetch(infoPeer);

        while (response.hasAttribute()) {
            uint32_t mtu, link;
            int attrType = response.getAttributeType();
            switch (attrType) {
                case IFLA_MTU:
                    response.fetch(IFLA_MTU, mtu);
                    attrs.push_back(make_tuple("mtu", std::to_string(mtu)));
                    break;
                case IFLA_LINK:
                    response.fetch(IFLA_LINK, link);
                    attrs.push_back(make_tuple("link", std::to_string(link)));
                    break;
                default:
                    response.skipAttribute();
                    break;
            }
        }
    } catch (const std::exception& ex) {
        LOGE(ex.what());
        throw VasumException(netdev + ": " + ex.what());
    }

    attrs.push_back(make_tuple("flags", std::to_string(infoPeer.ifi_flags)));
    attrs.push_back(make_tuple("type", std::to_string(infoPeer.ifi_type)));
    for (const auto& address : getIpAddresses(nsPid, AF_INET, infoPeer.ifi_index)) {
        attrs.push_back(make_tuple("ipv4", joinAddresses(address)));
    }

    for (const auto& address : getIpAddresses(nsPid, AF_INET6, infoPeer.ifi_index)) {
        attrs.push_back(make_tuple("ipv6", joinAddresses(address)));
    }

    return attrs;
}

void setAttrs(const pid_t nsPid, const std::string& netdev, const Attrs& attrs)
{
    const set<string> supportedAttrs{"flags", "change", "type", "mtu", "link", "ipv4", "ipv6"};

    LOGT("Setting network device informations: " << netdev);
    validateNetdevName(netdev);
    for (const auto& attr : attrs) {
        if (supportedAttrs.find(get<0>(attr)) == supportedAttrs.end()) {
            throw VasumException("Unsupported attribute: " + get<0>(attr));
        }
    }

    NetlinkMessage nlm(RTM_NEWLINK, NLM_F_REQUEST | NLM_F_CREATE | NLM_F_ACK);
    ifinfomsg infoPeer = utils::make_clean<ifinfomsg>();
    infoPeer.ifi_family = AF_UNSPEC;
    infoPeer.ifi_index = getInterfaceIndex(netdev, nsPid);
    infoPeer.ifi_change = 0xFFFFFFFF;
    for (const auto& attr : attrs) {
        if (get<0>(attr) == "flags") {
            infoPeer.ifi_flags = stoul(get<1>(attr));
        }
        if (get<0>(attr) == "change") {
            infoPeer.ifi_change = stoul(get<1>(attr));
        }
        if (get<0>(attr) == "type") {
            infoPeer.ifi_type = stoul(get<1>(attr));
        }
    }
    nlm.put(infoPeer);
    for (const auto& attr : attrs) {
        if (get<0>(attr) == "mtu") {
            nlm.put<uint32_t>(IFLA_MTU, stoul(get<1>(attr)));
        }
        if (get<0>(attr) == "link") {
            nlm.put<uint32_t>(IFLA_LINK, stoul(get<1>(attr)));
        }
    }

    NetlinkResponse response = send(nlm, nsPid);
    if (!response.hasMessage()) {
        throw VasumException("Can't set interface information");
    }

    //TODO: Multiple addresses should be set at once (add support NLM_F_MULTI to NetlinkMessage).
    vector<string> ipv4;
    vector<string> ipv6;
    for (const auto& attr : attrs) {
        if (get<0>(attr) == "ipv4") {
            ipv4.push_back(get<1>(attr));
        }
        if (get<0>(attr) == "ipv6") {
            ipv6.push_back(get<1>(attr));
        }
    }

    auto setIp = [nsPid](const vector<string>& ips, uint32_t index, int family) -> void {
        using namespace boost::algorithm;
        for (const auto& ip : ips) {
            Attrs attrs;
            vector<string> params;
            for (const auto& addrAttr : split(params, ip, is_any_of(","))) {
                size_t pos = addrAttr.find(":");
                if (pos == string::npos || pos == addrAttr.length()) {
                    const std::string msg = "Wrong input data format: ill formed address attribute: " + addrAttr;
                    LOGE(msg);
                    throw VasumException(msg);
                }
                attrs.push_back(make_tuple(addrAttr.substr(0, pos), addrAttr.substr(pos + 1)));
            }
            setIpAddresses(nsPid, index, attrs, family);
        }
    };

    setIp(ipv4, infoPeer.ifi_index, AF_INET);
    setIp(ipv6, infoPeer.ifi_index, AF_INET6);
}

void deleteIpAddress(const pid_t nsPid,
                     const std::string& netdev,
                     const std::string& ip)
{
    uint32_t index = getInterfaceIndex(netdev, nsPid);
    size_t slash = ip.find('/');
    if (slash == string::npos) {
        const std::string msg = "Wrong address format: it is not CIDR notation: can't find '/'";
        LOGE(msg);
        throw VasumException(msg);
    }
    int prefixlen = 0;
    try {
        prefixlen = stoi(ip.substr(slash + 1));
    } catch (const std::exception& ex) {
        const std::string msg = "Wrong address format: invalid prefixlen";
        LOGE(msg);
        throw VasumException(msg);
    }
    deleteIpAddress(nsPid, index, ip.substr(0, slash), prefixlen, getIpFamily(ip));
}


} //namespace netdev
} //namespace vasum

