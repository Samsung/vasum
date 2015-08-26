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

#include <vector>
#include <string>

#include <string.h>
#include <netinet/in.h>

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

/**
 * Unified ip address
 */
struct InetAddr {
    InetAddrType type;
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
        // TODO: Remove temporary usage
        (void) mType;
        (void) mMode;
    }

    void addNetAddr(const InetAddr&);
    void delNetAddr(const InetAddr&);

private:
    const std::string mHostIf;
    const std::string mZoneIf;
    const InterfaceType mType;
    const MacVLanMode mMode;
    std::vector<InetAddr> mIpAddrList;
};

} //namespace lxcpp

#endif // LXCPP_NETWORK_CONFIG_HPP
