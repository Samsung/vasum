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
 * @author  Lukasz Pawelczyk (l.pawelczyk@samsumg.com)
 * @brief   User namespace configuration implementation
 */

#include "lxcpp/userns-config.hpp"
#include "lxcpp/exception.hpp"

#include "logger/logger.hpp"

#include <vector>
#include <string>
#include <algorithm>
#include <tuple>


namespace lxcpp {


unsigned UserNSConfig::getContainerRootUID() const
{
    if (mUIDMaps.empty()) {
        return 0;
    }

    auto it = std::find_if(mUIDMaps.begin(), mUIDMaps.end(),
        [](const std::tuple<unsigned, unsigned, unsigned>& entry) {
            return std::get<0>(entry) == 0;
        }
    );

    if (it == mUIDMaps.end()) {
        const std::string msg = "The root UID is not mapped in the container";
        LOGE(msg);
        throw ConfigureException(msg);
    }

    return std::get<1>(*it);
}

unsigned UserNSConfig::getContainerRootGID() const
{
    if (mGIDMaps.empty()) {
        return 0;
    }

    auto it = std::find_if(mGIDMaps.begin(), mGIDMaps.end(),
        [](const std::tuple<unsigned, unsigned, unsigned>& entry) {
            return std::get<0>(entry) == 0;
        }
    );

    if (it == mGIDMaps.end()) {
        const std::string msg = "The root GID is not mapped in the container";
        LOGE(msg);
        throw ConfigureException(msg);
    }

    return std::get<1>(*it);
}


} //namespace lxcpp
