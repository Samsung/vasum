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

#include "config/fields.hpp"

#include <cstring>
#include <string>
#include <vector>
#include <ostream>

#include <arpa/inet.h>

namespace lxcpp {

std::string toString(const in_addr& addr);
std::string toString(const in6_addr& addr);
void fromString(const std::string& s, in_addr& addr);
void fromString(const std::string& s, in6_addr& addr);

/**
 * Suported address types
 */
enum class InetAddrType {
    IPV4,
    IPV6
};

/**
 * Unified ip address
 */
class InetAddr {
public:
    InetAddr() {}
    InetAddr(uint32_t flags, int prefix, const std::string& addr);

    InetAddrType getType() const {
        return static_cast<InetAddrType>(type);
    }
    void setType(InetAddrType t) {
        type = static_cast<int>(t);
    }

    template<typename T>
    T& getAddr() {
        //FIXME return union field after fix of addr type
        char *v = addr;
        return *(reinterpret_cast<T*>(v));
    }
    template<typename T>
    const T& getAddr() const {
        //FIXME return union field after fix of addr type
        const char *v = addr;
        return *(reinterpret_cast<const T*>(v));
    }

    uint32_t flags;
    int prefix;

    CONFIG_REGISTER
    (
        type,
        flags,
        prefix
        //FIXME add when visitor can serialize char[SIZE]
        //addr
    )

private:
 //FIXME change to union when visitor can serialize type by istream ostream operators
    char addr[sizeof(in6_addr)];
 //FIXME: change to enum when visitor can serialize type by istream ostream operators
    int type;
};

static inline bool operator==(const in_addr& a, const in_addr& b)
{
    return ::memcmp(&a, &b, sizeof(a)) == 0;
}

static inline bool operator==(const in6_addr& a, const in6_addr& b)
{
    return ::memcmp(&a, &b, sizeof(a)) == 0;
}

static inline bool operator==(const InetAddr& a, const InetAddr& b)
{
    if (a.getType() == b.getType() && a.prefix == b.prefix) {
        if (a.getType() == InetAddrType::IPV6)
            return a.getAddr<in6_addr>() == b.getAddr<in6_addr>();
        else
            return a.getAddr<in_addr>() == b.getAddr<in_addr>();
    }
    return false;
}

std::string toString(const InetAddr& a);

enum class RoutingTable {
    UNSPEC, // also means 'any'
    COMPAT,
    DEFAULT,
    MAIN,
    LOCAL,
    USER,
};
inline std::string toString(RoutingTable rt) {
    switch (rt) {
    case RoutingTable::UNSPEC:
        return "unspec";
    case RoutingTable::COMPAT:
        return "compat";
    case RoutingTable::DEFAULT:
        return "default";
    case RoutingTable::MAIN:
        return "main";
    case RoutingTable::LOCAL:
        return "local";
    default:
        return "user";
    }
}

enum class AttrName {
    MAC,
    FLAGS,
    CHANGE,
    TYPE,
    MTU,
    LINK,
    TXQLEN,
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
    }
    return os;
}

struct Attr {
    AttrName name;
    std::string value;
};

typedef std::vector<Attr> Attrs;

/**
 * Created interface type
 */
enum class InterfaceType : int {
    VETH,
    BRIDGE,
    MACVLAN
};

/**
 * Suported MacVLan modes
 */
enum class MacVLanMode {
    PRIVATE,
    VEPA,
    BRIDGE,
    PASSTHRU
};

enum class NetStatus {
    DOWN,
    UP
};

std::string toString(const InetAddr& a);

/**
 * Network operations to be performed on given container and interface
 * operates on netlink device
 */
class NetworkInterface {
public:
    /**
     * Create network interface object for the ifname in the container (network namespace)
     * Note: pid=0 is kernel
     */
    NetworkInterface(const std::string& ifname, pid_t pid = 0) :
        mIfname(ifname),
        mContainerPid(pid)
    {
    }

    const std::string& getName() const { return mIfname; }

    /**
     * Retrieve network interface status (UP or DOWN)
     */
    NetStatus status() const;

    /**
     * Create network interface in container identified by mContainerPid
     * Equivalent to: ip link add @mIfname type @type [...]
     *   Create pair of virtual ethernet interfaces
     *      ip link add @mIfname type veth peer name @peerif
     *   Create bridge interface
     *      ip link add @mIfname type bridge
     *   Create psedo-ethernet interface on existing one
     *      ip link add @mIfname type macvlan link @peerif [mode @a mode]
     */
    void create(InterfaceType type, const std::string& peerif, MacVLanMode mode = MacVLanMode::PRIVATE);

    /**
     * Delete interface
     * Equivalent to: ip link delete @mIfname
     */
    void destroy();

    /**
     * Move interface to container
     * Equivalent to: ip link set dev @hostif netns @mContainerPid
     */
    void moveToContainer(pid_t p);

    /**
     * Rename interface name
     * Equivalent to: ip link set dev @oldif name @mIfname
     */
    void renameFrom(const std::string& oldif);

    /**
     * Add interface to the bridge
     * Equivalent to: ip link set @mIfname master @bridge
     */
    void addToBridge(const std::string& bridge);

    /**
     * Remove insterface from the bridge
     * Equivalent to: ip link set @mIfname nomaster
     */
    void delFromBridge();

    /**
     * Set or get interface attributes in one netlink call.
     * Supported attributes: see @AttrNames
     */
    void setAttrs(const Attrs& attrs);
    Attrs getAttrs() const;

    /**
     * Add inet address to the interface
     * Equivalent to: ip addr add @addr dev @mIfname
     */
    void addInetAddr(const InetAddr& addr);

    /**
     * Remove inet address from the interface
     * Equivalent to: ip addr del @addr dev @mIfname
     */
    void delInetAddr(const InetAddr& addr);

    /**
     * Retrieve all inet addresses for the interface
     * Equivalent to: ip addr show, ip -6 addr show
     */
    std::vector<InetAddr> getInetAddressList() const;

    /**
     * Set interface up
     * Equivalent to: ip link set @mInface up
     */
    void up();

    /**
     * Set interface down
     * Equivalent to: ip link set @mInface down
     */
    void down();

    /**
     * Set MAC address attribute
     * Equivalent to: ip link set @mIface address @macaddr
     * @macaddr in format AA:BB:CC:DD:FF:GG
     */
    void setMACAddress(const std::string& macaddr);

    /**
     * Set MTU attribute
     * Equivalent to: ip link set @mIface mtu @mtu
     */
    void setMTU(int mtu);

    /**
     * Set TxQ attribute
     * Equivalent to: ip link set @mIface txqueue @txlen
     */
    void setTxLength(int txlen);

    /**
     * Get list of network interafece names
     * Equivalent to: ip link show
     */
    static std::vector<std::string> getInterfaces(pid_t initpid);

private:
    void createVeth(const std::string& peerif);
    void createBridge();
    void createMacVLan(const std::string& masterif, MacVLanMode mode);

    const std::string mIfname; ///< network interface name inside zone
    pid_t mContainerPid;       ///< Container pid to operate on (0 means kernel)
};

} // namespace lxcpp

#endif // LXCPP_NETWORK_HPP
