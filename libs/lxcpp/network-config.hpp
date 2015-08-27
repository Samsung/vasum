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
 * @brief   Network configuration classes
 */

#ifndef LXCPP_NETWORK_CONFIG_HPP
#define LXCPP_NETWORK_CONFIG_HPP

#include "config/config.hpp"
#include "config/fields.hpp"

#include <vector>
#include <string>

#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>

namespace lxcpp {

/**
 * Created interface type
 */
enum class InterfaceType {
    VETH,
    BRIDGE,
    MACVLAN,
    MOVE
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

/**
 * Suported address types
 */
enum class InetAddrType {
    IPV4,
    IPV6
};

enum class NetStatus {
    DOWN,
    UP
};


/**
 * Unified ip address
 */
struct InetAddr {
    InetAddrType type;
    uint32_t flags;
    int prefix;
    union {
        struct in_addr ipv4;
        struct in6_addr ipv6;
    } addr;
};

static inline bool operator==(const InetAddr& a, const InetAddr& b) {
    if (a.type == b.type && a.prefix == b.prefix) {
        if (a.type == InetAddrType::IPV4) {
            return ::memcmp(&a.addr.ipv4, &b.addr.ipv4, sizeof(a.addr.ipv4)) == 0;
        }
        if (a.type == InetAddrType::IPV6) {
            return ::memcmp(&a.addr.ipv6, &b.addr.ipv6, sizeof(a.addr.ipv6)) == 0;
        }
    }
    return false;
}


/**
 * Network interface configuration
 */
class NetworkInterfaceConfig {
public:
    NetworkInterfaceConfig(const std::string& hostif,
                           const std::string& zoneif,
                           InterfaceType type,
                           MacVLanMode mode = MacVLanMode::PRIVATE) :
        mHostIf(hostif),
        mZoneIf(zoneif),
        mType(type),
        mMode(mode)
    {
    }

    const std::string& getHostIf() const;

    const std::string& getZoneIf() const;

    const InterfaceType& getType() const;

    const MacVLanMode& getMode() const;

    const std::vector<InetAddr>& getAddrList() const;

    void addInetAddr(const InetAddr&);

private:
    const std::string mHostIf;
    const std::string mZoneIf;
    const InterfaceType mType;
    const MacVLanMode mMode;
    //TODO mtu, macaddress, txqueue
    /*
     * above are interface parameters which can be read/modified:
     *   MTU (Maximum Transmit Unit) is maximum length of link level packet in TCP stream
     *   MAC address is also called hardware card address
     *   TXQueue is transmit queue length
     *
     * I think most usufull would be possibility to set MAC address, other have their
     * well working defaults but can be tuned to make faster networking (especially localy)
     */
    std::vector<InetAddr> mIpAddrList;
};

/**
 * Network interface configuration
 */
struct NetworkConfig {

    //for convinience
    void addInterfaceConfig(const std::string& hostif,
                            const std::string& zoneif,
                            InterfaceType type,
                            MacVLanMode mode);
    void addInetConfig(const std::string& ifname, const InetAddr& addr);

    std::vector<NetworkInterfaceConfig> mInterfaces;

    //TODO tmporary to allow serialization of this object
    CONFIG_REGISTER_EMPTY
};

} //namespace lxcpp

#endif // LXCPP_NETWORK_CONFIG_HPP
