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
 * @brief   Process credentials handling
 */

#ifndef LXCPP_CREDENTIALS_HPP
#define LXCPP_CREDENTIALS_HPP

#include <sys/types.h>

#include <vector>

namespace lxcpp {

void setgroups(const std::vector<gid_t>& groups);

void setgid(const gid_t gid);

void setuid(const uid_t uid);

} // namespace lxcpp

#endif // LXCPP_CREDENTIALS_HPP