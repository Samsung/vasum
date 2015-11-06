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
 * @brief   IPC messages declaration
 */


#ifndef LXCPP_GUARD_API_HPP
#define LXCPP_GUARD_API_HPP

#include "ipc/types.hpp"

#include "cargo/fields.hpp"

#include <string>
#include <vector>

namespace lxcpp {
namespace api {

const ::ipc::MethodID METHOD_SET_CONFIG  = 1;
const ::ipc::MethodID METHOD_GET_CONFIG  = 2;
const ::ipc::MethodID METHOD_START       = 3;
const ::ipc::MethodID METHOD_STOP        = 4;
const ::ipc::MethodID METHOD_GUARD_READY = 5;

struct Void {
    CARGO_REGISTER_EMPTY
};

struct Pid {
    int value;

    Pid(int v = -1): value(v) {}

    CARGO_REGISTER
    (
        value
    )
};

} // namespace api
} // namespace lxcpp

#endif // LXCPP_GUARD_API_HPP
