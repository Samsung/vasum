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
#include "lxcpp/guard/api.hpp"

#include "utils/channel.hpp"
#include "cargo-ipc/client.hpp"

#include <sys/types.h>
#include <memory>


namespace lxcpp {

/**
 * Starts the container. Assumes container isn't already running.
 *
 * Prepares an environment for a guard process,
 * starts it, and controls it with RPC.
 *
 * After execute() object will live till all it's callbacks are run.
 */
class Start final: public Command {
public:

    /**
     * @param config container's config
     */
    Start(std::shared_ptr<ContainerConfig>& config);
    ~Start();

    void execute();

private:
    std::shared_ptr<ContainerConfig> mConfig;
    std::string mGuardPath;

    void parent(const pid_t pid);
    void daemonize();
};


} // namespace lxcpp


#endif // LXCPP_COMMANDS_START_HPP
