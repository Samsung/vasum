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
 * @brief   Implementation of user namespace setup
 */

#include "lxcpp/commands/setup-userns.hpp"

#include "logger/logger.hpp"
#include "utils/fs.hpp"

#include <vector>
#include <iostream>


namespace lxcpp {


SetupUserNS::SetupUserNS(UserNSConfig &userNSConfig, pid_t initPid)
    : mUserNSConfig(userNSConfig),
      mInitPid(initPid)
{
}

SetupUserNS::~SetupUserNS()
{
}

void SetupUserNS::execute()
{
    const std::string proc = "/proc/" + std::to_string(mInitPid);
    const std::string uid_map_path = proc + "/uid_map";
    const std::string gid_map_path = proc + "/gid_map";

    std::string uid_map;
    for (const auto map : mUserNSConfig.mUIDMaps) {
        uid_map.append(std::to_string(std::get<0>(map)));
        uid_map.append(" ");
        uid_map.append(std::to_string(std::get<1>(map)));
        uid_map.append(" ");
        uid_map.append(std::to_string(std::get<2>(map)));
        uid_map.append("\n");
    }
    if (uid_map.size() && !utils::saveFileContent(uid_map_path, uid_map)) {
        const std::string msg = "Failed to write the uid_map";
        LOGE(msg);
        throw UserNSException(msg);
    }

    std::string gid_map;
    for (const auto map : mUserNSConfig.mGIDMaps) {
        gid_map.append(std::to_string(std::get<0>(map)));
        gid_map.append(" ");
        gid_map.append(std::to_string(std::get<1>(map)));
        gid_map.append(" ");
        gid_map.append(std::to_string(std::get<2>(map)));
        gid_map.append("\n");
    }
    if (gid_map.size() && !utils::saveFileContent(gid_map_path, gid_map)) {
        const std::string msg = "Failed to write the gid_map";
        LOGE(msg);
        throw UserNSException(msg);
    }
}


} // namespace lxcpp
