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

#include "config.hpp"

#include "lxcpp/cgroups/devices.hpp"
#include "lxcpp/exception.hpp"
#include "utils/text.hpp"
#include "logger/logger.hpp"

#ifdef USE_BOOST_REGEX
#include <boost/regex.hpp>
namespace rgx = boost;
#else
#include <regex>
namespace rgx = std;
#endif

namespace lxcpp {

namespace {
std::string devString(int n)
{
    return  n >= -1 ? std::to_string(n) : "*";
}

DevicePermission& parsePerms(DevicePermission& p,const std::string& line)
{
    std::string re = "^([a-z]) ([0-9]+|\\*):([0-9]+|\\*) ([a-z]+)$";
    rgx::smatch match;
    try {
        rgx::regex rgex(re);
        if (!rgx::regex_search(line, match, rgex)) {
            throw CGroupException("wrong input: " + line);
        }
    } catch (CGroupException) {
        throw;
    } catch (std::runtime_error e) {
        throw std::runtime_error(e.what() + std::string(" update your c++ libs"));
    }

    p.type = match.str(1).at(0);
    p.major = match.str(2) == "*" ? -1 : std::stoi(match.str(2));
    p.minor = match.str(3) == "*" ? -1 : std::stoi(match.str(3));
    p.permission = match.str(4);
    return p;
}

} //namespace

void DevicesCGroup::allow(DevicePermission p)
{
    allow(p.type, p.major, p.minor, p.permission);
}

void DevicesCGroup::deny(DevicePermission p)
{
    deny(p.type, p.major, p.minor, p.permission);
}

void DevicesCGroup::allow(char type, int major, int minor, const std::string& perm)
{
    setValue("allow", type + std::string(" ") + devString(major) + ":" + devString(minor) + " " + perm);
}

void DevicesCGroup::deny(char type, int major, int minor, const std::string& perm)
{
    setValue("deny", type + std::string(" ") + devString(major) + ":" + devString(minor) + " " + perm);
}

std::vector<DevicePermission> DevicesCGroup::list()
{
    std::vector<DevicePermission> list;
    DevicePermission p;
    for (const auto& ln : utils::split(getValue("list"), "\n")) {
        if (!ln.empty()) {
            list.push_back(parsePerms(p, ln));
        }
    }
    return list;
}

} //namespace lxcpp
