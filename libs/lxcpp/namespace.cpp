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
 * @author  Jan Olszak (j.olszak@samsung.com)
 * @brief   lxcpp container factory definition
 */

#include "lxcpp/namespace.hpp"
#include "lxcpp/exception.hpp"
#include "logger/logger.hpp"

#include <numeric>
#include <functional>

namespace lxcpp {

Namespace operator|(const Namespace a, const Namespace b)
{
    return static_cast<Namespace>(static_cast<std::underlying_type<Namespace>::type>(a) |
                                  static_cast<std::underlying_type<Namespace>::type>(b));
}

std::string toString(const Namespace ns)
{
    switch(ns) {
    case Namespace::USER:
        return "user";
    case Namespace::MNT:
        return "mnt";
    case Namespace::PID:
        return "pid";
    case Namespace::UTS:
        return "uts";
    case Namespace::IPC:
        return "ipc";
    case Namespace::NET:
        return "net";
    default:
        const std::string msg = "Bad namespace passed to the function";
        LOGE(msg);
        throw BadArgument(msg);
    }
}

int toFlag(const std::vector<Namespace>& namespaces)
{
    Namespace flag = std::accumulate(namespaces.begin(),
                                     namespaces.end(),
                                     static_cast<Namespace>(0),
                                     std::bit_or<Namespace>());
    return static_cast<int>(flag);
}

int toFlag(const Namespace ns)
{
    return static_cast<int>(ns);
}

std::string getNsPath(const pid_t pid)
{
    return "/proc/" + std::to_string(pid) + "/ns";
}

std::string getPath(const pid_t pid, const Namespace ns)
{
    return getNsPath(pid) + "/" + toString(ns);
}

} // namespace lxcpp
