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

#include "lxcpp/network-config.hpp"
#include "lxcpp/exception.hpp"
#include "logger/logger.hpp"
#include <algorithm>

namespace lxcpp {

const std::string& NetworkInterfaceConfig::getHostIf() const
{
    return mHostIf;
}

const std::string& NetworkInterfaceConfig::getZoneIf() const
{
    return mZoneIf;
}

InterfaceConfigType NetworkInterfaceConfig::getType() const
{
    return static_cast<InterfaceConfigType>(mType);
}

MacVLanMode NetworkInterfaceConfig::getMode() const
{
    return static_cast<MacVLanMode>(mMode);
}

void NetworkInterfaceConfig::setMTU(int mtu)
{
    mMtu = mtu;
}

void NetworkInterfaceConfig::setMACAddress(const std::string& mac)
{
    mMacAddress = mac;
}

void NetworkInterfaceConfig::setTxLength(int txlen)
{
    mTxLength = txlen;
}

int NetworkInterfaceConfig::getMTU() const
{
    return mMtu;
}

const std::string& NetworkInterfaceConfig::getMACAddress() const
{
    return mMacAddress;
}

int NetworkInterfaceConfig::getTxLength() const
{
    return mTxLength;
}

const std::vector<InetAddr>& NetworkInterfaceConfig::getAddrList() const
{
    return mIpAddrList;
}

void NetworkInterfaceConfig::addInetAddr(const InetAddr& addr)
{
    std::vector<InetAddr>::iterator exists = std::find(mIpAddrList.begin(), mIpAddrList.end(), addr);
    if (exists != mIpAddrList.end()) {
        const std::string msg("Address already assigned");
        throw NetworkException(msg);
    }
    mIpAddrList.push_back(addr);
}

void NetworkConfig::addInterfaceConfig(InterfaceConfigType type,
                                       const std::string& hostif,
                                       const std::string& zoneif,
                                       const std::vector<InetAddr>& addrs,
                                       MacVLanMode mode)
{
    auto it = std::find_if(mInterfaces.begin(), mInterfaces.end(),
        [&hostif,&zoneif](const NetworkInterfaceConfig& entry) {
            return entry.getHostIf() == hostif && entry.getZoneIf() == zoneif;
        }
    );

    if (it != mInterfaces.end()) {
        const std::string msg = "Interface already exists";
        LOGE(msg);
        throw NetworkException(msg);
    }
    mInterfaces.push_back(NetworkInterfaceConfig(type, hostif, zoneif, addrs, mode));
}

void NetworkConfig::addInetConfig(const std::string& ifname, const InetAddr& addr)
{
    auto it = std::find_if(mInterfaces.begin(), mInterfaces.end(),
        [&ifname](const NetworkInterfaceConfig& entry) {
            return entry.getZoneIf() == ifname || entry.getHostIf() == ifname;
        }
    );

    if (it == mInterfaces.end()) {
        const std::string msg = "No such interface";
        LOGE(msg);
        throw NetworkException(msg);
    }
    it->addInetAddr(addr);
}

} //namespace lxcpp
