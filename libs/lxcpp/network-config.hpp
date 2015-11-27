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

#include "config.hpp"
#include "cargo/fields.hpp"
#include "lxcpp/network.hpp"
#include "lxcpp/exception.hpp"

#include <vector>
#include <string>

#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>

namespace lxcpp {

enum class InterfaceConfigType : int {
    LOOPBACK,
    BRIDGE,
    VETH_BRIDGED
};

/**
 * Network interface configuration
 */
class NetworkInterfaceConfig {
public:
    NetworkInterfaceConfig() = default;  // default constructor required by visitor

    NetworkInterfaceConfig(InterfaceConfigType type,
                           const std::string& hostif,
                           const std::string& zoneif,
                           const std::vector<InetAddr>& addrs,
                           MacVLanMode mode) :
        mHostIf(hostif),
        mZoneIf(zoneif),
        mType(static_cast<int>(type)),
        mMode(static_cast<int>(mode)),
        mMtu(0),
        mMacAddress(),
        mTxLength(0),
        mIpAddrList(addrs)
    {
    }

    const std::string& getHostIf() const;

    const std::string& getZoneIf() const;

    InterfaceConfigType getType() const;

    MacVLanMode getMode() const;

    void setMTU(int mtu);
    int getMTU() const;

    void setMACAddress(const std::string& mac);
    const std::string& getMACAddress() const;

    void setTxLength(int txlen);
    int getTxLength() const;

    const std::vector<InetAddr>& getAddrList() const;

    void addInetAddr(const InetAddr& addr);

    CARGO_REGISTER
    (
        mHostIf,
        mZoneIf,
        mType,
        mMode,
        mIpAddrList
    )

private:
    std::string mHostIf;
    std::string mZoneIf;
    int mType;
    int mMode;

    /*
     * interface parameters which can be set after interface is created
     *   MTU (Maximum Transmit Unit) is maximum length of link level packet in TCP stream
     *   MAC address is also called hardware card address
     *   TXQueue is transmit queue length
     *
     * I think most useful would be possibility to set MAC address, other have their
     * well working defaults but can be tuned to make faster networking (especially locally)
     */
    int mMtu;
    std::string mMacAddress;
    int mTxLength;

    std::vector<InetAddr> mIpAddrList;
};

/**
 * Network interface configuration
 */
class NetworkConfig {
public:
    /**
     * adds interface configuration.
     * throws NetworkException if zoneif name already on list
     */
    void addInterfaceConfig(InterfaceConfigType type,
                            const std::string& hostif,
                            const std::string& zoneif = "",
                            const std::vector<InetAddr>& addrs = std::vector<InetAddr>(),
                            MacVLanMode mode = MacVLanMode::PRIVATE);
    void addInetConfig(const std::string& ifname, const InetAddr& addr);

    const std::vector<NetworkInterfaceConfig>& getInterfaces() const { return mInterfaces; }
    const NetworkInterfaceConfig& getInterface(int i) const { return mInterfaces.at(i); }

    CARGO_REGISTER(
        mInterfaces
    )

private:
    std::vector<NetworkInterfaceConfig> mInterfaces;
};

} //namespace lxcpp

#endif // LXCPP_NETWORK_CONFIG_HPP
