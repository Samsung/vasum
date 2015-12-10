/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Lukasz Kostyra <l.kostyra@samsung.com>
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
 * @author  Lukasz Kostyra (l.kostyra@samsung.com)
 * @brief   VT-related utility functions
 */

#include "config.hpp"

#include "utils/vt.hpp"
#include "logger/logger.hpp"
#include "utils/exception.hpp"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/vt.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

namespace {

const std::string TTY_DEV = "/dev/tty0";

} // namespace

namespace utils {

bool activateVT(const int& vt)
{
    int consoleFD = ::open(TTY_DEV.c_str(), O_WRONLY);
    if (consoleFD < 0) {
        LOGE("console open failed: " << errno << " (" << getSystemErrorMessage() << ")");
        return false;
    }

    struct vt_stat vtstat;
    vtstat.v_active = 0;
    if (::ioctl(consoleFD, VT_GETSTATE, &vtstat)) {
        LOGE("Failed to get vt state: " << errno << " (" << getSystemErrorMessage() << ")");
        ::close(consoleFD);
        return false;
    }

    if (vtstat.v_active == vt) {
        LOGW("vt" << vt << " is already active.");
        ::close(consoleFD);
        return true;
    }

    // activate vt
    if (::ioctl(consoleFD, VT_ACTIVATE, vt)) {
        LOGE("Failed to activate vt" << vt << ": " << errno << " (" << getSystemErrorMessage() << ")");
        ::close(consoleFD);
        return false;
    }

    // wait until activation is finished
    if (::ioctl(consoleFD, VT_WAITACTIVE, vt)) {
        LOGE("Failed to wait for vt" << vt << " activation: " << errno << " (" << getSystemErrorMessage() << ")");
        ::close(consoleFD);
        return false;
    }

    ::close(consoleFD);
    return true;
}

} // namespace utils
