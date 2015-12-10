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
 * @brief   Linux capabilities handling routines
 */

#include "config.hpp"

#include "lxcpp/capability.hpp"
#include "lxcpp/exception.hpp"

#include "logger/logger.hpp"
#include "utils/exception.hpp"

#include <unistd.h>
#include <sys/types.h>
#include <linux/capability.h>
#include <sys/prctl.h>

#include <fstream>

namespace lxcpp {

namespace {

int getLastCap()
{
    std::ifstream ifs("/proc/sys/kernel/cap_last_cap");
    if (!ifs.good()) {
        const std::string msg = "Failed to open /proc/sys/kernel/cap_last_cap";
        LOGE(msg);
        throw CapabilitySetupException(msg);
    }

    int lastCap;
    ifs >> lastCap;

    return lastCap;
}

} // namespace

void dropCapsFromBoundingExcept(unsigned long long mask)
{
    // This is thread safe in C++11
    static int lastCap  = getLastCap();

    // Drop caps except those in the mask
    for (int cap = 0; cap <= lastCap; ++cap) {
        if (mask & (1LL << cap))
            continue;

        if (::prctl(PR_CAPBSET_DROP, cap, 0, 0, 0)) {
            const std::string msg = "Failed to remove capability id: " + std::to_string(cap) +
                                    ", error: " + utils::getSystemErrorMessage();
            LOGE(msg);
            throw ProcessSetupException(msg);
        }
    }
}
} // namespace lxcpp
