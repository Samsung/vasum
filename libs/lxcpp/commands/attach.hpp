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
 * @brief   Implementation of attaching to a container
 */

#ifndef LXCPP_COMMANDS_ATTACH_HPP
#define LXCPP_COMMANDS_ATTACH_HPP

#include "lxcpp/commands/command.hpp"
#include "lxcpp/attach/attach-config.hpp"
#include "lxcpp/container-impl.hpp"
#include "utils/channel.hpp"

#include <sys/types.h>

#include <string>

namespace lxcpp {

class Attach final: Command {
public:

    /**
     * Runs call in the container's context
     *
     * Object attach should be used immediately after creation.
     * It will not be stored for future use like most other commands.
     *
     * @param container container to which it attaches
     * @param argv path and arguments for the user's executable
     * @param uid uid in the container
     * @param gid gid in the container
     * @param ttyPath path of the TTY in the host
     * @param supplementaryGids supplementary groups in container
     * @param capsToKeep capabilities that will be kept
     * @param workDirInContainer work directory set for the new process
     * @param envToKeep environment variables that will be kept
     * @param envToSet new environment variables that will be set
     */
    Attach(const lxcpp::ContainerImpl& container,
           const std::vector<const char*>& argv,
           const uid_t uid,
           const gid_t gid,
           const std::string& ttyPath,
           const std::vector<gid_t>& supplementaryGids,
           const int capsToKeep,
           const std::string& workDirInContainer,
           const std::vector<std::string>& envToKeep,
           const std::vector<std::pair<std::string, std::string>>& envToSet);
    ~Attach();

    void execute();

private:
    utils::Channel mIntermChannel;
    AttachConfig mConfig;

    void parent(const pid_t pid);

};

} // namespace lxcpp

#endif // LXCPP_COMMANDS_ATTACH_HPP