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
#include "utils.hpp"
#include "exception.hpp"

#include <logger/logger.hpp>

#include <algorithm>
#include <string>
#include <cstdint>
#include <cstring>
#include <cassert>

#include <net/if.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <ifaddrs.h>
#include <linux/rtnetlink.h>
#include <linux/veth.h>
#include <linux/sockios.h>
#include <linux/if_link.h>
#include <linux/rtnetlink.h>
#include <linux/in6.h>
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
using namespace vasum;
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
        LOGE("Can't get " << name << " interface index (" << getSystemErrorMessage() << ")");
        throw ZoneOperationException("Can't find interface");
    }
    return index;
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
        LOGE("Can't open socket (" << getSystemErrorMessage() << ")");
        throw ZoneOperationException("Can't attach to bridge");
    }

    struct ifreq ifr = utils::make_clean<ifreq>();
    strncpy(ifr.ifr_name, bridge.c_str(), IFNAMSIZ);
    ifr.ifr_ifindex = index;
    int err = ioctl(fd, SIOCBRADDIF, &ifr);
    if (err < 0) {
        int error = errno;
        //TODO: Close can be interrupted. Move util functions from ipc
        ::close(fd);
        LOGE("Can't attach to bridge (" + getSystemErrorMessage(error) + ")");
        throw ZoneOperationException("Can't attach to bridge");
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


} //namespace netdev
} //namespace vasum

