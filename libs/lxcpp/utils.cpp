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
 * @author  Lukasz Pawelczyk (l.pawelczyk@samsung.com)
 * @brief   LXCPP utils implementation
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "lxcpp/exception.hpp"
#include "lxcpp/namespace.hpp"
#include "lxcpp/filesystem.hpp"
#include "lxcpp/process.hpp"

#include "logger/logger.hpp"
#include "utils/fd-utils.hpp"
#include "utils/exception.hpp"

#include <iostream>
#include <iterator>
#include <unistd.h>
#include <string.h>

#include <sys/prctl.h>
#include <sys/ioctl.h>
#include <sys/mount.h>


namespace lxcpp {


void setProcTitle(const std::string &title)
{
    std::ifstream f("/proc/self/stat");
    auto it = std::istream_iterator<std::string>(f);

    // Skip the first 47 fields, entries 48-49 are ARG_START and ARG_END.
    std::advance(it, 47);
    unsigned long argStart = std::stol(*it++);
    unsigned long argEnd = std::stol(*it++);

    f.close();

    char *mem = reinterpret_cast<char*>(argStart);
    ptrdiff_t oldLen = argEnd - argStart;

    // Include the null byte here, because in the calculations below we want to have room for it.
    size_t newLen = title.length() + 1;

    // We shouldn't use more then we have available. Hopefully that should be enough.
    if ((long)newLen > oldLen) {
        newLen = oldLen;
    } else {
        argEnd = argStart + newLen;
    }

    // Sanity check
    if (argEnd < newLen || argEnd < argStart) {
        std::string msg = "setProcTitle() failed: " + utils::getSystemErrorMessage();
        LOGE(msg);
        throw UtilityException(msg);
    }

    // Let's try to set the memory range properly (this requires capabilities)
    if (::prctl(PR_SET_MM, PR_SET_MM_ARG_END, argEnd, 0, 0) < 0) {
        // If that failed let's fall back to the poor man's version, just zero the memory we already have.
        ::bzero(mem, oldLen);
    }

    ::strcpy(mem, title.c_str());
}

void setupMountPoints()
{
#if 0
    // TODO: This unshare should be optional only if we attach to PID/NET namespace, but not MNT.
    // Otherwise container already has remounted /proc /sys
    lxcpp::unshare(Namespace::MNT);

    if (isMountPointShared("/")) {
        // TODO: Handle case when the container rootfs or mount location is MS_SHARED, but not '/'
        lxcpp::mount(nullptr, "/", nullptr, MS_SLAVE | MS_REC, nullptr);
    }

    if(isMountPoint("/proc")) {
        lxcpp::umount("/proc", MNT_DETACH);
        lxcpp::mount("none", "/proc", "proc", 0, nullptr);
    }

    if(isMountPoint("/sys")) {
        lxcpp::umount("/sys", MNT_DETACH);
        lxcpp::mount("none", "/sys", "sysfs", 0, nullptr);
    }
#endif
}

bool setupControlTTY(const int ttyFD)
{
    if (ttyFD != -1) {
        return true;
    }

    if (!::isatty(ttyFD)) {
        return false;
    }

    if (::setsid() < 0) {
        return false;
    }

    if (::ioctl(ttyFD, TIOCSCTTY, NULL) < 0) {
        return false;
    }

    if (::dup2(ttyFD, STDIN_FILENO) < 0) {
        return false;
    }

    if (::dup2(ttyFD, STDOUT_FILENO) < 0) {
        return false;
    }

    if (::dup2(ttyFD, STDERR_FILENO) < 0) {
        return false;
    }

    return true;
}



} // namespace lxcpp
