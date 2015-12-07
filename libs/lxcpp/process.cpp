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

#include "config.hpp"

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

#include <array>

namespace lxcpp {

pid_t fork()
{
    pid_t pid = ::fork();
    if (pid < 0) {
        const std::string msg = "fork() failed: " +
                                utils::getSystemErrorMessage();
        LOGE(msg);
        throw ProcessSetupException(msg);
    }
    return pid;
}

pid_t clone(int (*function)(void *),
            void *args,
            const int flags)
{
    // Won't fail, well known resource name
    size_t stackSize = ::sysconf(_SC_PAGESIZE);

    // PAGESIZE is enough, it'll exec after this
    char *stack = static_cast<char*>(::alloca(stackSize));

    pid_t pid = ::clone(function, stack  + stackSize, flags | SIGCHLD, args);
    if (pid < 0) {
        const std::string msg = "clone() failed: " +
                                utils::getSystemErrorMessage();
        LOGE(msg);
        throw ProcessSetupException(msg);
    }

    return pid;
}

void setns(const pid_t pid, int requestedNamespaces)
{
    int dirFD = utils::open(getNsPath(pid), O_DIRECTORY | O_CLOEXEC);

    static const std::array<int, 6> NAMESPACES {{
            CLONE_NEWUSER, CLONE_NEWNS, CLONE_NEWPID, CLONE_NEWUTS, CLONE_NEWIPC, CLONE_NEWNET
        }};

    // Open FDs connected with the requested namespaces
    std::vector<int> fds;
    for(const int ns: NAMESPACES) {
        if (!(ns & requestedNamespaces)) {
            // This namespace wasn't requested
            continue;
        }

        int fd = ::openat(dirFD,
                          nsToString(ns).c_str(),
                          O_RDONLY | O_CLOEXEC);
        if(fd < 0) {
            const std::string msg = "openat() failed: " + utils::getSystemErrorMessage();

            // Cleanup file descriptors
            for (const int d: fds) {
                utils::close(d);
            }
            utils::close(dirFD);

            LOGE(msg);
            throw ProcessSetupException(msg);
        }

        fds.push_back(fd);
    }

    // Setns to every requested namespace
    for(size_t i = 0; i < fds.size(); ++i) {
        if(-1 == ::setns(fds[i], 0 /* we're sure it's a fd of the right namespace*/)) {
            const std::string msg = "setns() failed: " + utils::getSystemErrorMessage();

            for (size_t j = i; j < fds.size(); ++j) {
                utils::close(fds[j]);
            }
            utils::close(dirFD);

            LOGE(msg);
            throw ProcessSetupException(msg);
        }
        utils::close(fds[i]);
    }

    utils::close(dirFD);
}

int waitpid(const pid_t pid)
{
    int status;
    while (-1 == ::waitpid(pid, &status, 0)) {
        if (errno == EINTR) {
            LOGT("waitpid() interrupted, retrying");
            continue;
        }
        const std::string msg = "waitpid() failed: " + utils::getSystemErrorMessage();
        LOGE(msg);
        throw ProcessSetupException(msg);
    }

    // Return child's return status if everything is OK
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }

    // Something went wrong in the child
    std::string msg;
    if (WIFSIGNALED(status)) {
        msg = "Child killed by signal " + std::to_string(WTERMSIG(status));
    } else {
        msg = "Unknown error in child process";
    }
    LOGE(msg);
    throw ProcessSetupException(msg);
}

void unshare(const int ns)
{
    if (-1 == ::unshare(ns)) {
        const std::string msg = "unshare() failed: " + utils::getSystemErrorMessage();
        LOGE(msg);
        throw ProcessSetupException(msg);
    }
}

void execv(const utils::CArgsBuilder& argv)
{
    // Run user's binary
    ::execv(argv[0], const_cast<char *const*>(argv.c_array()));

    const std::string msg = "execv() failed: " + utils::getSystemErrorMessage();
    LOGE(msg);
    throw ProcessSetupException(msg);
}

void execve(const utils::CArgsBuilder& argv)
{
    // Run user's binary
    ::execve(argv[0], const_cast<char *const*>(argv.c_array()), nullptr);

    const std::string msg = "execve() failed: " + utils::getSystemErrorMessage();
    LOGE(msg);
    throw ProcessSetupException(msg);
}

} // namespace lxcpp
