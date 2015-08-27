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
 * @brief   Network configuration command
 */

#include "lxcpp/commands/netcreate.hpp"
#include "lxcpp/network.hpp"

namespace lxcpp {

void NetCreateAll::execute()
{
    for (const auto& i : mInterfaceConfigs) {
        NetworkInterface networkInerface(mContainerPid, i.getZoneIf());
        networkInerface.create(i.getHostIf(), i.getType(), i.getMode());

        Attrs attrs;
        for (const auto& a : i.getAddrList()) {
            Attr attr;
            NetworkInterface::convertInetAddr2Attr(a, attr);
            attrs.push_back(attr);
        }
        networkInerface.setAttrs(attrs);
    }
}

} // namespace lxcpp
