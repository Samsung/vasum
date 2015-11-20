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
 * @brief   Implementation of stopping a container
 */

#ifndef LXCPP_COMMANDS_STOP_HPP
#define LXCPP_COMMANDS_STOP_HPP

#include "lxcpp/commands/command.hpp"
#include "lxcpp/container-config.hpp"

#include "cargo-ipc/client.hpp"

#include <sys/types.h>
#include <memory>


namespace lxcpp {


class Stop final: public Command {
public:
    /**
     * Stops the container
     *
     * @param config container's config
     */
    Stop(std::shared_ptr<ContainerConfig>& config,
         std::shared_ptr<cargo::ipc::Client>& client);
    ~Stop();

    void execute();

private:
    std::shared_ptr<ContainerConfig> mConfig;
    std::shared_ptr<cargo::ipc::Client> mClient;
};


} // namespace lxcpp


#endif // LXCPP_COMMANDS_STOP_HPP
