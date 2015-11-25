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
 * @brief   Linux hostname handling routines
 */

#include "config.hpp"

#include "exception.hpp"
#include "logger/logger.hpp"
#include "utils/exception.hpp"

#include <string>
#include <unistd.h>
#include <limits.h>

namespace lxcpp {

void setHostName(const std::string& hostname)
{
    if (hostname.empty()) {
        const std::string msg = "HostName cannot be empty";
        LOGE(msg);
        throw BadArgument(msg);
    }

    // sethostname needs CAP_SYS_ADMIN capability in UTS namespace
    if (-1 == ::sethostname(hostname.c_str(), hostname.length())) {
        const std::string msg = "Failed to set hostname: " + hostname +
                                ", error: " + utils::getSystemErrorMessage();
        LOGE(msg);
        throw BadArgument(msg);
    }
}

std::string getHostName()
{
    char hostname[HOST_NAME_MAX + 1];

    if (-1 == ::gethostname(hostname, sizeof hostname)) {
        const std::string msg = "Failed to get hostname, error: " + utils::getSystemErrorMessage();
        LOGE(msg);
        throw NoSuchValue(msg);
    }

    return std::string(hostname);
}

} // namespace lxcpp
