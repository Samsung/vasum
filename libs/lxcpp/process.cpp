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
#include "utils/fd-utils.hpp"
#include "utils/exception.hpp"

#include <alloca.h>
#include <sched.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

namespace lxcpp {

pid_t clone(int (*function)(void *),
            void *args,
            const std::vector<Namespace>& namespaces,
            const int additionalFlags)
{
    // Won't fail, well known resource name
    size_t stackSize = ::sysconf(_SC_PAGESIZE);

    // PAGESIZE is enough, it'll exec after this
    char *stack = static_cast<char*>(::alloca(stackSize));

    pid_t pid = ::clone(function, stack  + stackSize, toFlag(namespaces) | additionalFlags | SIGCHLD, args);
    if (pid < 0) {
        const std::string msg = utils::getSystemErrorMessage();
        LOGE("clone() failed: " << msg);
        throw ProcessSetupException("clone() failed " + msg);
    }

    return pid;
}

void setns(const std::vector<Namespace>& namespaces)
{
    pid_t pid = ::getpid();

    int dirFD = ::open(getNsPath(pid).c_str(), O_DIRECTORY | O_CLOEXEC);
    if(dirFD < 0) {
        const std::string msg = utils::getSystemErrorMessage();
        LOGE("open() failed: " << msg);
        throw ProcessSetupException("open() failed: " + msg);
    }

    // Open FDs connected with the requested namespaces
    std::vector<int> fds(namespaces.size(), -1);
    for(size_t i = 0; i < namespaces.size(); ++i) {
        fds[i] = ::openat(dirFD, toString(namespaces[i]).c_str(), O_RDONLY | O_CLOEXEC);
        if(fds[i] < 0) {
            const std::string msg = utils::getSystemErrorMessage();

            for (size_t j = 0; j < i; ++j) {
                utils::close(fds[j]);
            }
            utils::close(dirFD);

            LOGE("openat() failed: " << msg);
            throw ProcessSetupException("openat() failed: " + msg);
        }
    }

    // Setns for every namespace
    for(size_t i = 0; i < fds.size(); ++i) {
        if(-1 == ::setns(fds[i], toFlag(namespaces[i]))) {
            const std::string msg = utils::getSystemErrorMessage();

            for (size_t j = i; j < fds.size(); ++j) {
                utils::close(fds[j]);
            }
            utils::close(dirFD);

            LOGE("setns() failed: " << msg);
            throw ProcessSetupException("setns() failed: " + msg);
        }
        utils::close(fds[i]);
    }

    utils::close(dirFD);
}

} // namespace lxcpp