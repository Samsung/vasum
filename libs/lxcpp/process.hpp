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

#ifndef LXCPP_PROCESS_HPP
#define LXCPP_PROCESS_HPP

#include "lxcpp/namespace.hpp"
#include "utils/c-args.hpp"

#include <sys/types.h>
#include <vector>

namespace lxcpp {

pid_t fork();

pid_t clone(int (*function)(void *),
            void *args,
            const int flags);

void setns(const pid_t pid, const int namespaces);

int waitpid(const pid_t pid);

void unshare(const int ns);

void execve(const utils::CArgsBuilder& argv);

} // namespace lxcpp

#endif // LXCPP_PROCESS_HPP
