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
 * @brief   Control-groups configuration
 */

#ifndef LXCPP_CGROUPS_CGROUP_CONFIG_HPP
#define LXCPP_CGROUPS_CGROUP_CONFIG_HPP

#include "config/config.hpp"
#include "config/fields.hpp"

#include <string>
#include <vector>

namespace lxcpp {

struct SubsystemConfig {
    std::string name;
    std::string path;

    CONFIG_REGISTER
    (
        name,
        path
    )
};

struct CGroupParam {
    std::string name;
    std::string value;

    CONFIG_REGISTER
    (
        name,
        value
    )
};

struct CGroupConfig {
    std::string subsystem;
    std::string name;
    std::vector<CGroupParam> common; // cgroup.*
    std::vector<CGroupParam> params; // {name}.*

    CONFIG_REGISTER
    (
        subsystem,
        name,
        common,
        params
    )
};

struct CGroupsConfig {
    std::vector<SubsystemConfig> subsystems;
    std::vector<CGroupConfig> cgroups;

    CONFIG_REGISTER
    (
        subsystems,
        cgroups
    )
};

} // namespace lxcpp

#endif //LXCPP_CGROUPS_CGROUP_CONFIG_HPP
