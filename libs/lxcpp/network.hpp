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

#include "config.hpp"
#include <cargo/fields.hpp>

#include <cstring>
#include <string>
#include <vector>
#include <array>
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
    InetAddr() = default;
    InetAddr(const std::string& addr, unsigned prefix, uint32_t flags=0);

    template<typename T>
    T& getAddr() {
        std::uint8_t *v = addr.data();
        return *(reinterpret_cast<T*>(v));
    }
    template<typename T>
    const T& getAddr() const {
        const std::uint8_t *v = addr.data();
        return *(reinterpret_cast<const T*>(v));
    }

    InetAddrType type;
    unsigned prefix;
    uint32_t flags;

    CARGO_REGISTER
    (
        type,
        flags,
        prefix,
        addr
    )

private:
    std::array<std::uint8_t,sizeof(in6_addr)> addr;
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
    if (a.type == b.type && a.prefix == b.prefix) {
        if (a.type == InetAddrType::IPV6)
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

inline std::string toString(const RoutingTable rt)
{
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

struct Route {
    InetAddr dst;
    InetAddr src;
    unsigned metric;
    std::string ifname;
    RoutingTable table;
};

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
    //TODO implement Netlink singleton per pid
public:
    /**
     * Create network interface object for the @b ifname in the container (network namespace)
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
     * Create network interface in container identified by @ref mContainerPid.
     *
     * Equivalent to: ip link add @ref mIfname type @b type [...]
     *
     *   Create pair of virtual ethernet interfaces:
     *      - ip link add @ref mIfname type veth peer name @b peerif
     *
     *   Create bridge interface:
     *      - ip link add @ref mIfname type bridge
     *
     *   Create pseudo-ethernet interface on existing one:
     *      - ip link add @ref mIfname type macvlan link @b peerif [mode @b mode]
     */
    void create(InterfaceType type, const std::string& peerif = "", MacVLanMode mode = MacVLanMode::PRIVATE);

    /**
     * Delete interface.
     * Equivalent to: ip link delete @ref mIfname
     */
    void destroy();

    /**
     * Move interface to container.
     * Equivalent to: ip link set dev @ref mIfname netns @b pid
     */
    void moveToContainer(pid_t pid);

    /**
     * Rename interface name.
     * Equivalent to: ip link set dev @b oldif name @ref mIfname
     */
    void renameFrom(const std::string& oldif);

    /**
     * Add interface to the bridge.
     * Equivalent to: ip link set @ref mIfname master @b bridge
     */
    void addToBridge(const std::string& bridge);

    /**
     * Remove insterface from the bridge.
     * Equivalent to: ip link set @ref mIfname nomaster
     */
    void delFromBridge();

    /**
     * Set or get interface attributes in one netlink call.
     * Supported attributes: see @ref AttrName
     */
    void setAttrs(const Attrs& attrs);
    Attrs getAttrs() const;

    /**
     * Add inet address to the interface.
     * Equivalent to: ip addr add @b addr dev @ref mIfname
     */
    void addInetAddr(const InetAddr& addr);

    /**
     * Remove inet address from the interface.
     * Equivalent to: ip addr del @b addr dev @ref mIfname
     */
    void delInetAddr(const InetAddr& addr);

    /**
     * Retrieve all inet addresses for the interface.
     * Equivalent to: ip addr show, ip -6 addr show
     */
    std::vector<InetAddr> getInetAddressList() const;

    /**
     * Add route to specified routing table.
     * Equivalent to: ip route add @b route.dst.addr/@b route.dst.prefix dev @ref mIfname (if route.src.prefix=0)
     */
    void addRoute(const Route& route, const RoutingTable rt = RoutingTable::MAIN);

    /**
     * Remove route from specified routing table.
     * Equivalent to: ip route del @b route.dst.addr dev @ref mIfname
     */
    void delRoute(const Route& route, const RoutingTable rt = RoutingTable::MAIN);

    /**
     * Retrieve routing table for the interface.
     * Equivalent to: ip route show dev @ref mIfname table @b rt
     */
    std::vector<Route> getRoutes(const RoutingTable rt = RoutingTable::MAIN) const;

    /**
     * Set interface up.
     * Equivalent to: ip link set @ref mIfname up
     */
    void up();

    /**
     * Set interface down.
     * Equivalent to: ip link set @ref mIfname down
     */
    void down();

    /**
     * Set MAC address attribute.
     * Equivalent to: ip link set @ref mIfname address @b macaddr (@b macaddr in format AA:BB:CC:DD:FF:GG)
     *
     * Note: two lower bits of first byte (leftmost) specifies MAC address class:
     *      - b1: 0=unicast, 1=broadcast
     *      - b2: 0=global,  1=local
     *
     * In most cases should be b2=0, b1=1, (see: https://en.wikipedia.org/wiki/MAC_address)
     */
    void setMACAddress(const std::string& macaddr);

    /**
     * Set MTU attribute.
     * Equivalent to: ip link set @ref mIfname mtu @b mtu
     */
    void setMTU(int mtu);

    /**
     * Set TxQ attribute.
     * Equivalent to: ip link set @ref mIfname txqueue @b txlen
     */
    void setTxLength(int txlen);

    /**
     * Get list of network interafece names.
     * Equivalent to: ip link show
     */
    static std::vector<std::string> getInterfaces(pid_t initpid);

    /**
     * Get list of routes (specified routing table).
     * Equivalent to: ip route show table @b rt
     */
    static std::vector<Route> getRoutes(pid_t initpid, const RoutingTable rt = RoutingTable::MAIN);

private:
    void createVeth(const std::string& peerif);
    void createBridge();
    void createMacVLan(const std::string& masterif, MacVLanMode mode);

    void modifyRoute(int cmd, const InetAddr& src, const InetAddr& dst);

    const std::string mIfname; ///< network interface name inside zone
    pid_t mContainerPid;       ///< Container pid to operate on (0 means kernel)
};

} // namespace lxcpp

#endif // LXCPP_NETWORK_HPP
