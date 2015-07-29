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
 * @brief   Headers for starting a container
 */

#ifndef LXCPP_COMMANDS_START_HPP
#define LXCPP_COMMANDS_START_HPP

#include "lxcpp/commands/command.hpp"
#include "lxcpp/container-config.hpp"
#include "utils/channel.hpp"

#include <sys/types.h>


namespace lxcpp {


class Start final: Command {
public:
    /**
     * Starts the container
     *
     * In more details it prepares an environment for a guard process,
     * starts it, and passes it the configuration through a file descriptor.
     *
     * @param config container's config
     */
    Start(ContainerConfig &config);
    ~Start();

    void execute();

private:
    ContainerConfig &mConfig;
    utils::Channel mChannel;
    std::string mGuardPath;
    std::string mChannelFD;

    void parent(const pid_t pid);
    void daemonize();
};


} // namespace lxcpp


#endif // LXCPP_COMMANDS_START_HPP
