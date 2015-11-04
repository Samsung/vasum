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
class Start final: public Command, public std::enable_shared_from_this<Start> {
public:

    /**
     * @param config container's config
     * @param client IPC connection to the Guard process
     */
    Start(std::shared_ptr<ContainerConfig>& config,
          std::shared_ptr<cargo::ipc::Client>& client);
    ~Start();

    void execute();

    /**
     * Guards tells that it's ready to receive commands.
     *
     * This is a method handler, not signal to avoid races.
     *
     * @param client IPC connection to the Guard process
     * @param config container's config
     */
    bool onGuardReady(const cargo::ipc::PeerID,
                      std::shared_ptr<api::Void>&,
                      cargo::ipc::MethodResult::Pointer);

private:
    std::shared_ptr<ContainerConfig> mConfig;
    std::string mGuardPath;
    std::shared_ptr<cargo::ipc::Client> mClient;

    void parent(const pid_t pid);
    void daemonize();
};


} // namespace lxcpp


#endif // LXCPP_COMMANDS_START_HPP
