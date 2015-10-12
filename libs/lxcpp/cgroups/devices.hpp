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
 * @brief   Control-groups management, devices
 */

#ifndef LXCPP_CGROUPS_DEVICES_HPP
#define LXCPP_CGROUPS_DEVICES_HPP

#include "lxcpp/cgroups/cgroup.hpp"

namespace lxcpp {

struct DevicePermission {
    char type;              // 'a'==any, 'b'==block, 'c'==character
    int major,minor;        // -1==any
    std::string permission; // combination of "rwm" (r==read,w==write,m==create)
};

class DevicesCGroup : public CGroup {
public:
    DevicesCGroup(const std::string& name) :
        CGroup("devices", name)
    {
    }

    void allow(DevicePermission p);
    void deny(DevicePermission p);
    void allow(char type, int major, int minor, const std::string& perm);
    void deny(char type, int major, int minor, const std::string& perm);
    std::vector<DevicePermission> list();
};

} //namespace lxcpp

#endif //LXCPP_CGROUPS_DEVICE_HPP
