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

#include "lxcpp/exception.hpp"

#include "logger/logger.hpp"
#include "utils/fd-utils.hpp"
#include "utils/exception.hpp"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <iostream>
#include <iterator>
#include <unistd.h>
#include <string.h>
#include <sys/prctl.h>


namespace lxcpp {


/*
 * This function has to be safe in regard to signal(7)
 */
int nullStdFDs()
{
    int ret = -1;

    int fd = TEMP_FAILURE_RETRY(open("/dev/null", O_RDWR | O_CLOEXEC));
    if (fd == -1) {
        goto err;
    }

    if (-1 == TEMP_FAILURE_RETRY(::dup2(fd, STDIN_FILENO))) {
        goto err_close;
    }

    if (-1 == TEMP_FAILURE_RETRY(::dup2(fd, STDOUT_FILENO))) {
        goto err_close;
    }

    if (-1 == TEMP_FAILURE_RETRY(::dup2(fd, STDERR_FILENO))) {
        goto err_close;
    }

    if (-1 == TEMP_FAILURE_RETRY(::close(fd))) {
        goto err_close;
    }

    return 0;

err_close:
    TEMP_FAILURE_RETRY(::close(fd));
err:
    return ret;
}

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


} // namespace lxcpp
