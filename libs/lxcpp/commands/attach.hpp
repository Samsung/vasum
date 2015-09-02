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
#include "lxcpp/container-impl.hpp"
#include "utils/channel.hpp"

#include <string>

namespace lxcpp {

class Attach final: Command {
public:
    typedef std::function<int(void)> Call;

    /**
     * Runs call in the container's context
     *
     * Object attach should be used immediately after creation.
     * It will not be stored for future use like most other commands.
     *
     * @param container container to which it attaches
     * @param userCall user's function to run
     * @param capsToKeep capabilities that will be kept
     * @param workDirInContainer work directory set for the new process
     * @param envToKeep environment variables that will be kept
     * @param envToSet new environment variables that will be set
     */
    Attach(lxcpp::ContainerImpl& container,
           Container::AttachCall& userCall,
           const int capsToKeep,
           const std::string& workDirInContainer,
           const std::vector<std::string>& envToKeep,
           const std::vector<std::pair<std::string, std::string>>& envToSet);
    ~Attach();

    void execute();

private:
    const lxcpp::ContainerImpl& mContainer;
    const Container::AttachCall& mUserCall;
    const int mCapsToKeep;
    const std::string& mWorkDirInContainer;
    const std::vector<std::string>& mEnvToKeep;
    const std::vector<std::pair<std::string, std::string>>& mEnvToSet;

    // Methods for different stages of setting up the attachment
    static int child(const Container::AttachCall& call,
                     const int capsToKeep,
                     const std::vector<std::string>& envToKeep,
                     const std::vector<std::pair<std::string, std::string>>& envToSet);

    void parent(utils::Channel& intermChannel,
                const pid_t pid);

    void interm(utils::Channel& intermChannel,
                Call& call);
};

} // namespace lxcpp

#endif // LXCPP_COMMANDS_ATTACH_HPP