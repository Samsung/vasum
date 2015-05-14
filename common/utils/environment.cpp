/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Michal Witanowski <m.witanowski@samsung.com>
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License
 */

/**
 * @file
 * @author  Michal Witanowski (m.witanowski@samsung.com)
 * @brief   Implementaion of environment setup routines that require root privileges
 */

#include "config.hpp"

#include "utils/environment.hpp"
#include "utils/execute.hpp"
#include "utils/exception.hpp"
#include "utils/make-clean.hpp"
#include "base-exception.hpp"
#include "logger/logger.hpp"

#include <cap-ng.h>
#include <grp.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cstring>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <map>
#include <cassert>
#include <features.h>

#if !__GLIBC_PREREQ(2, 14)

#include <sys/syscall.h>

#ifdef __NR_setns
static inline int setns(int fd, int nstype)
{
    // setns system call are available since v2.6.39-6479-g7b21fdd
    return syscall(__NR_setns, fd, nstype);
}
#else
#error "setns syscall isn't available"
#endif

#endif

using namespace utils;

namespace {

const std::map<int, std::string> NAMESPACES = {
    {CLONE_NEWIPC, "ipc"},
    {CLONE_NEWNET, "net"},
    {CLONE_NEWNS, "mnt"},
    {CLONE_NEWPID, "pid"},
    {CLONE_NEWUSER, "user"},
    {CLONE_NEWUTS, "uts"}};

int fdRecv(int socket)
{
    msghdr msg = make_clean<msghdr>();
    iovec iov = make_clean<iovec>();
    char cmsgBuff[CMSG_SPACE(sizeof(int))];

    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    msg.msg_control = cmsgBuff;
    msg.msg_controllen = sizeof(cmsgBuff);

    int ret = recvmsg(socket, &msg, MSG_CMSG_CLOEXEC);
    if (ret != 0 || msg.msg_flags & (MSG_TRUNC | MSG_ERRQUEUE | MSG_OOB | MSG_CTRUNC | MSG_EOR)) {
        LOGE("Can't receive fd: ret: " << ret << ", flags: " << msg.msg_flags);
        return -1;
    }

    cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
    assert(cmsg->cmsg_level == SOL_SOCKET);
    assert(cmsg->cmsg_type == SCM_RIGHTS);
    assert(CMSG_NXTHDR(&msg, cmsg) == NULL);
    return *reinterpret_cast<int*>(CMSG_DATA(cmsg));
}

bool fdSend(int socket, int fd)
{
    msghdr msg = make_clean<msghdr>();
    struct iovec iov = make_clean<iovec>();
    struct cmsghdr *cmsg = NULL;
    char cmsgBuff[CMSG_SPACE(sizeof(int))];

    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    msg.msg_control = cmsgBuff;
    msg.msg_controllen = sizeof(cmsgBuff);

    cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(int));
    *reinterpret_cast<int*>(CMSG_DATA(cmsg)) = fd;

    int ret = sendmsg(socket, &msg, 0);
    if (ret < 0) {
        LOGE("Can't send fd: ret: " << ret);
        return false;
    }
    return true;
}

} // namespace

namespace utils {


bool setSuppGroups(const std::vector<std::string>& groups)
{
    std::vector<gid_t> gids;

    for (const std::string& group : groups) {
        // get GID from name
        struct group* grp = ::getgrnam(group.c_str());
        if (grp == NULL) {
            LOGE("getgrnam failed to find group '" << group << "'");
            return false;
        }

        LOGD("'" << group << "' group ID: " << grp->gr_gid);
        gids.push_back(grp->gr_gid);
    }

    if (::setgroups(gids.size(), gids.data()) != 0) {
        LOGE("setgroups() failed: " << getSystemErrorMessage());
        return false;
    }

    return true;
}

bool dropRoot(uid_t uid, gid_t gid, const std::vector<unsigned int>& caps)
{
    ::capng_clear(CAPNG_SELECT_BOTH);

    for (const auto cap : caps) {
        if (::capng_update(CAPNG_ADD, static_cast<capng_type_t>(CAPNG_EFFECTIVE |
                                                                CAPNG_PERMITTED |
                                                                CAPNG_INHERITABLE), cap)) {
            LOGE("Failed to set capability: " << ::capng_capability_to_name(cap));
            return false;
        }
    }

    if (::capng_change_id(uid, gid, static_cast<capng_flags_t>(CAPNG_CLEAR_BOUNDING))) {
        LOGE("Failed to change process user");
        return false;
    }

    return true;
}

bool launchAsRoot(const std::function<bool()>& func)
{
    if (::getuid() == 0) {
        // we are already root, no need to fork
        return func();
    } else {
        return executeAndWait([&func]() {
            if (::setuid(0) < 0) {
                LOGW("Failed to become root: " << getSystemErrorMessage());
                _exit(EXIT_FAILURE);
            }

            if (!func()) {
                LOGE("Failed to successfully execute func");
                _exit(EXIT_FAILURE);
            }
        });
    }
}

bool joinToNs(int nsPid, int ns)
{
    auto ins = NAMESPACES.find(ns);
    if (ins == NAMESPACES.end()) {
        LOGE("Namespace isn't supported: " << ns);
        return false;
    }
    std::string nsPath = "/proc/" + std::to_string(nsPid) + "/ns/" + ins->second;
    int nsFd = ::open(nsPath.c_str(), O_RDONLY);
    if (nsFd == -1) {
        LOGE("Can't open namesace: " + getSystemErrorMessage());
        return false;
    }
    int ret = setns(nsFd, ins->first);
    if (ret != 0) {
        LOGE("Can't set namesace: " + getSystemErrorMessage());
        close(nsFd);
        return false;
    }
    close(nsFd);
    return true;
}

int passNemaspacedFd(int nsPid, int ns, const std::function<int()>& fdFactory)
{
    int fds[2];
    int ret = socketpair(PF_LOCAL, SOCK_RAW, 0, fds);
    if (ret == -1) {
        LOGE("Can't create socket pair: " << getSystemErrorMessage());
        return -1;
    }
    bool success = executeAndWait([&, fds, nsPid, ns]() {
        close(fds[0]);

        int fd = -1;
        if (joinToNs(nsPid, ns)) {
            fd = fdFactory();
        }
        if (fd == -1) {
            close(fds[1]);
            _exit(EXIT_FAILURE);
        }
        LOGT("FD pass, send: " << fd);
        fdSend(fds[1], fd);
        close(fds[1]);
        close(fd);
    });

    close(fds[1]);
    int fd = -1;
    if (success) {
        fd = fdRecv(fds[0]);
    }
    close(fds[0]);
    LOGT("FD pass, rcv: " << fd);
    return fd;
}

} // namespace utils
