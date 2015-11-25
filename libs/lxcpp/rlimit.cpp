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
 * @author  Dariusz Michaluk (d.michaluk@samsung.com)
 * @brief   Linux resource limits handling routines
 */

#include "config.hpp"

#include "exception.hpp"
#include "rlimit.hpp"
#include "logger/logger.hpp"
#include "utils/exception.hpp"

namespace lxcpp {

void setRlimit(const int type, const uint64_t soft, const uint64_t hard)
{
    if (type < 0 || soft > hard || soft > RLIM_INFINITY || hard > RLIM_INFINITY) {
        const std::string msg = "Incorrect type, hard or soft limit";
        LOGE(msg);
        throw BadArgument(msg);
    }

    // Unprivileged process may set soft limit (range from 0 up to the hard limit) and irreversibly lower hard limit
    // Privileged process (with the CAP_SYS_RESOURCE capability) may make arbitrary changes to either limit value
    struct rlimit rlim;
    rlim.rlim_cur = soft;
    rlim.rlim_max = hard;

    if (-1 == ::setrlimit(type, &rlim)) {
        const std::string msg = "Failed to set resource limit, error: " + utils::getSystemErrorMessage();
        LOGE(msg);
        throw BadArgument(msg);
    }
}

struct rlimit getRlimit(const int type)
{
    struct rlimit rlim;

    if (-1 == ::getrlimit(type, &rlim)) {
        const std::string msg = "Failed to get resource limit, error: " + utils::getSystemErrorMessage();
        LOGE(msg);
        throw NoSuchValue(msg);
    }

    return rlim;
}

} // namespace lxcpp
