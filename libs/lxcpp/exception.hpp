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
 * @author  Mateusz Malicki (m.malicki2@samsung.com)
 * @brief   lxcpp's exceptions definitions
 */

#ifndef LXCPP_EXCEPTION_HPP
#define LXCPP_EXCEPTION_HPP

#include <stdexcept>

namespace lxcpp {

class ContainerException : std::runtime_error
{
public:
    ContainerException(const std::string& what) : std::runtime_error(what) {}
};

class NotImplemented : ContainerException
{
public:
    NotImplemented() : ContainerException(std::string()) {}
};

} // namespace lxcpp

#endif // LXCPP_EXCEPTION_HPP
