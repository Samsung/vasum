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
 * @brief   Process credentials handling
 */

#include "lxcpp/credentials.hpp"
#include "lxcpp/exception.hpp"

#include "logger/logger.hpp"
#include "utils/exception.hpp"

#include <unistd.h>
#include <grp.h>

namespace lxcpp {

void setgroups(const std::vector<gid_t>& gids)
{
    if(-1 == ::setgroups(gids.size(), gids.data())) {
        const std::string msg = "setgroups() failed: " +
                                utils::getSystemErrorMessage();
        LOGE(msg);
        throw CredentialSetupException(msg);
    }
}

void setregid(const gid_t rgid, const gid_t egid)
{
    if(-1 == ::setregid(rgid, egid)) {
        const std::string msg = "setregid() failed: " +
                                utils::getSystemErrorMessage();
        LOGE(msg);
        throw CredentialSetupException(msg);
    }
}

void setreuid(const uid_t ruid, const uid_t euid)
{
    if(-1 == ::setreuid(ruid, euid)) {
        const std::string msg = "setreuid() failed: " +
                                utils::getSystemErrorMessage();
        LOGE(msg);
        throw CredentialSetupException(msg);
    }
}

pid_t setsid()
{
    pid_t pid = ::setsid();
    if (pid == -1) {
        const std::string msg = "setsid() failed: " +
                                utils::getSystemErrorMessage();
        LOGE(msg);
        throw CredentialSetupException(msg);
    }
    return pid;
}

} // namespace lxcpp

