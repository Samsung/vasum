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
 * @brief   Handling environment variables
 */

#ifndef LXCPP_ENVIRONMENT_HPP
#define LXCPP_ENVIRONMENT_HPP

#include <vector>
#include <utility>
#include <string>

namespace lxcpp {

void clearenv();

/**
 * Clears the env variables except those listed.
 * There's a race condition - a moment when listed variables aren't set
 * Function should be used only for setting up a new process.
 *
 * @param names names of the variables to keep
 */
void clearenvExcept(const std::vector<std::string>& names);

std::string getenv(const std::string& name);

void setenv(const std::string& name, const std::string& value);

void setenv(const std::vector<std::pair<std::string, std::string>>& variables);

} // namespace lxcpp

#endif // LXCPP_ENVIRONMENT_HPP
