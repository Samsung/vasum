/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Piotr Bartosiewicz <p.bartosiewi@partner.samsung.com>
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
 * @author  Piotr Bartosiewicz (p.bartosiewi@partner.samsung.com)
 * @brief   Api for talking to init via initctl
 */

#include "config.hpp"

#include "utils/initctl.hpp"

#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

namespace utils {

namespace {
    struct InitctlRequest {
        int magic;
        int cmd;
        int runlevel;
        int sleeptime;
        char data[368];
    };
    const int INITCTL_MAGIC = 0x03091969;
    const int INITCTL_CMD_RUNLVL = 1;

    bool write(int fd, const void* data, size_t size)
    {
        while (size > 0) {
            ssize_t r = ::write(fd, data, size);
            if (r < 0) {
                if (errno == EINTR) {
                    continue;
                }
                return false;
            }
            size -= static_cast<size_t>(r);
            data = reinterpret_cast<const char*>(data) + r;
        }
        return true;
    }

    void close(int fd)
    {
        while (::close(fd) == -1 && errno == EINTR) {}
    }
}

bool setRunLevel(RunLevel runLevel)
{
    int fd = ::open("/dev/initctl", O_WRONLY|O_NONBLOCK|O_CLOEXEC|O_NOCTTY);
    if (fd < 0) {
        return false;
    }

    InitctlRequest req;
    memset(&req, 0, sizeof(req));
    req.magic = INITCTL_MAGIC;
    req.cmd = INITCTL_CMD_RUNLVL;
    req.runlevel = '0' + runLevel;
    req.sleeptime = 0;

    bool ret = write(fd, &req, sizeof(req));
    close(fd);
    return ret;
}


} // namespace utils
