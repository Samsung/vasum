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
#include <limits.h>


namespace lxcpp {


void UserNSConfig::addUIDMap(uid_t contID, uid_t hostID, unsigned num)
{
    assertMapCorrect(mUIDMaps, "UID", contID, hostID, num);
    mUIDMaps.emplace_back(contID, hostID, num);
}

void UserNSConfig::addGIDMap(gid_t contID, gid_t hostID, unsigned num)
{
    assertMapCorrect(mGIDMaps, "GID", contID, hostID, num);
    mGIDMaps.emplace_back(contID, hostID, num);
}

uid_t UserNSConfig::convContToHostUID(uid_t contID) const
{
    return convContToHostID(mUIDMaps, "UID", contID);
}

gid_t UserNSConfig::convContToHostGID(gid_t contID) const
{
    return convContToHostID(mGIDMaps, "GID", contID);
}

void UserNSConfig::assertMapCorrect(const IDMap &map, const std::string &ID,
                                    unsigned contID, unsigned hostID, unsigned num) const
{
    if (map.size() >= 5) {
        const std::string msg = "Max number of 5 " + ID + " mappings has been already reached";
        LOGE(msg);
        throw ConfigureException(msg);
    }

    uid_t max = (uid_t)-1;
    if ((contID > (max - num)) || (hostID > (max - num))) {
        const std::string msg = "Given " + ID + " range exceeds maximum allowed values";
        LOGE(msg);
        throw ConfigureException(msg);
    }

    if (map.size()) {
        for (auto &it : map) {
            unsigned size = std::get<2>(it);
            uid_t contMin = std::get<0>(it);
            uid_t contMax = contMin + size - 1;
            uid_t hostMin = std::get<1>(it);
            uid_t hostMax = hostMin + size - 1;

            if ((contMin <= contID && contID <= contMax) ||
                (hostMin <= hostID && hostID <= hostMax)) {
                const std::string msg = "Given " + ID + " range overlaps with already configured mappings";
                LOGE(msg);
                throw ConfigureException(msg);
            }
        }
    }
}

unsigned UserNSConfig::convContToHostID(const IDMap &map, const std::string &ID,
                                        unsigned contID) const
{
    if (map.empty()) {
        return contID;
    }

    auto it = std::find_if(map.begin(), map.end(),
        [contID](const std::tuple<unsigned, unsigned, unsigned>& entry) {
            unsigned size = std::get<2>(entry);
            uid_t contMin = std::get<0>(entry);
            uid_t contMax = contMin + size - 1;

            if (contMin <= contID && contID <= contMax) {
                return true;
            }

            return false;
        }
    );

    if (it == map.end()) {
        const std::string msg = "The " + ID + ": " + std::to_string(contID) + " is not mapped in the container";
        LOGE(msg);
        throw ConfigureException(msg);
    }

    unsigned diff = contID - std::get<0>(*it);
    unsigned hostID = std::get<1>(*it) + diff;

    return hostID;
}


} //namespace lxcpp
