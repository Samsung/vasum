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

#ifndef LXCPP_NETWORK_HPP
#define LXCPP_NETWORK_HPP

#include "lxcpp/network-config.hpp"

#include <string>
#include <vector>
#include <ostream>

namespace lxcpp {

enum class AttrName {
    MAC,
    FLAGS,
    CHANGE,
    TYPE,
    MTU,
    LINK,
    TXQLEN,
    IPV4,
    IPV6,
};

inline std::ostream& operator<<(std::ostream& os, const AttrName& a) {
    switch (a) {
        case AttrName::MAC: os << "mac"; break;
        case AttrName::FLAGS: os << "flags"; break;
        case AttrName::CHANGE: os << "change"; break;
        case AttrName::TYPE: os << "type"; break;
        case AttrName::MTU: os << "mtu"; break;
        case AttrName::LINK: os << "link"; break;
        case AttrName::TXQLEN: os << "txq"; break;
        case AttrName::IPV4: os << "ipv4"; break;
        case AttrName::IPV6: os << "ipv6"; break;
    }
    return os;
}

struct Attr {
    AttrName name;
    std::string value;
};

typedef std::vector<Attr> Attrs;

/**
 * Network operations to be performed on given container and interface
 * operates on netlink device
 */
class NetworkInterface {
public:
    /**
     * Create network interface object for the ifname in the container
     */
    NetworkInterface(pid_t pid, const std::string& ifname) :
        mContainerPid(pid),
        mIfname(ifname)
    {
    }

    const std::string& getName() const { return mIfname; }

    //Network actions on Container
    void create(const std::string& hostif, InterfaceType type, MacVLanMode mode=MacVLanMode::PRIVATE);
    void destroy();

    NetStatus status();
    void up();
    void down();

    /**
     * Rename interface name
     * Equivalent to: ip link set dev $oldif name $this.mIfname
     */
    void renameFrom(const std::string& oldif);

    void setAttrs(const Attrs& attrs);
    Attrs getAttrs() const;

    void setMACAddress(const std::string& macaddr);
    void setMTU(int mtu);
    void setTxLength(int txlen);
    void addInetAddr(const InetAddr& addr);
    void delInetAddr(const InetAddr& addr);

    static std::vector<std::string> getInterfaces(pid_t initpid);

    static void convertInetAddr2Attr(const InetAddr& addr, Attr& attr);
    static bool convertAttr2InetAddr(const Attr& attr, InetAddr& addr);

private:
    void createVeth(const std::string& hostif);
    void createBridge(const std::string& hostif);
    void createMacVLan(const std::string& hostif, MacVLanMode mode);
    /**
     * Move interface to container
     * Equivalent to: ip link set dev $hostif netns $mContainerPid
     */
    void move(const std::string& hostif);

    pid_t mContainerPid;       ///< Container pid to operate on (0=kernel)
    const std::string mIfname; ///< network interface name inside zone
};

} // namespace lxcpp

#endif // LXCPP_NETWORK_HPP
