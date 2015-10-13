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

namespace lxcpp {

std::string nsToString(const int ns)
{
    switch(ns) {
    case CLONE_NEWUSER:
        return "user";
    case CLONE_NEWNS:
        return "mnt";
    case CLONE_NEWPID:
        return "pid";
    case CLONE_NEWUTS:
        return "uts";
    case CLONE_NEWIPC:
        return "ipc";
    case CLONE_NEWNET:
        return "net";
    default:
        const std::string msg = "Bad namespace passed to the function";
        LOGE(msg);
        throw BadArgument(msg);
    }
}

std::string getNsPath(const pid_t pid)
{
    return "/proc/" + std::to_string(pid) + "/ns";
}

std::string getPath(const pid_t pid, const int ns)
{
    return getNsPath(pid) + "/" + nsToString(ns);
}

} // namespace lxcpp
