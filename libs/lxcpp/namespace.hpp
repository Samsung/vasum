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

namespace lxcpp {

std::string nsToString(const int ns);

std::string getNsPath(const pid_t pid);

std::string getPath(const pid_t pid, const int ns);

} // namespace lxcpp

#endif // LXCPP_NAMESPACE_HPP