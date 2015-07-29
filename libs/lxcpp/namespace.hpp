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
 * @brief   process handling routines
 */

#ifndef LXCPP_NAMESPACE_HPP
#define LXCPP_NAMESPACE_HPP

#include <sched.h>
#include <string>
#include <vector>

namespace lxcpp {

enum class Namespace : int {
    USER = CLONE_NEWUSER,
    MNT = CLONE_NEWNS,
    PID = CLONE_NEWPID,
    UTS = CLONE_NEWUTS,
    IPC = CLONE_NEWIPC,
    NET = CLONE_NEWNET
};

Namespace operator |(const Namespace a, const Namespace b);

std::string toString(const Namespace ns);

std::string getNsPath(const pid_t pid);

std::string getPath(const pid_t pid, const Namespace ns);

int toFlag(const Namespace ns);

int toFlag(const std::vector<Namespace>& namespaces);

} // namespace lxcpp

#endif // LXCPP_NAMESPACE_HPP