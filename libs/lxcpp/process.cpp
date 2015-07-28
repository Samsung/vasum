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

#include "lxcpp/process.hpp"
#include "lxcpp/exception.hpp"
#include "logger/logger.hpp"

#include <alloca.h>
#include <sched.h>
#include <unistd.h>
#include <sys/wait.h>

namespace lxcpp {

pid_t clone(int (*function)(void *), int flags, void *args) {
    // Won't fail, well known resource name
    size_t stackSize = ::sysconf(_SC_PAGESIZE);

    // PAGESIZE is enough, it'll exec after this
    char *stack = static_cast<char*>(::alloca(stackSize));

    pid_t ret;
    ret = ::clone(function, stack  + stackSize, flags | SIGCHLD, args);
    if (ret < 0) {
        LOGE("clone() failed");
        throw ProcessSetupException("clone() failed");
    }

    return ret;
}

} // namespace lxcpp