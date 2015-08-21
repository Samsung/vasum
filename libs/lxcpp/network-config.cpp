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
#include <algorithm>

namespace lxcpp {

void NetworkInterfaceConfig::addNetAddr(const InetAddr& addr)
{
    std::vector<InetAddr>::iterator exists = std::find(mIpAddrList.begin(), mIpAddrList.end(), addr);
    if (exists != mIpAddrList.end()) {
        std::string msg("Address alredy assigned");
        throw NetworkException(msg);
    }
    mIpAddrList.push_back(addr);
}

void NetworkInterfaceConfig::delNetAddr(const InetAddr& addr)
{
    std::vector<InetAddr>::iterator exists = std::find(mIpAddrList.begin(), mIpAddrList.end(), addr);
    mIpAddrList.erase(exists);
}

} //namespace lxcpp
