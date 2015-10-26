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
 * @brief   Headers of user namespace setup
 */

#ifndef LXCPP_COMMANDS_SETUP_USERNS_HPP
#define LXCPP_COMMANDS_SETUP_USERNS_HPP

#include "lxcpp/commands/command.hpp"
#include "lxcpp/container-config.hpp"


namespace lxcpp {


class SetupUserNS final: Command {
public:
    /**
     * Setups the user namespace by filling UID/GID mappings
     *
     * @param userNSConfig  A config containing UID and GID mappings
     */
    SetupUserNS(UserNSConfig &userNSConfig, pid_t initPid);
    ~SetupUserNS();

    void execute();

private:
    UserNSConfig &mUserNSConfig;
    pid_t mInitPid;
};


} // namespace lxcpp


#endif // LXCPP_COMMANDS_SETUP_USERNS_HPP
