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

#include "lxcpp/commands/attach.hpp"
#include "lxcpp/exception.hpp"
#include "lxcpp/process.hpp"

#include "utils/exception.hpp"
#include "utils/fd-utils.hpp"
#include "logger/logger.hpp"

#include <unistd.h>

namespace lxcpp {

Attach::Attach(const lxcpp::ContainerImpl& container,
               const std::vector<std::string>& argv,
               const uid_t uid,
               const gid_t gid,
               const std::string& ttyPath,
               const std::vector<gid_t>& supplementaryGids,
               const int capsToKeep,
               const std::string& workDirInContainer,
               const std::vector<std::string>& envToKeep,
               const std::vector<std::pair<std::string, std::string>>& envToSet)
    : mIntermChannel(false),
      mConfig(argv,
              container.getInitPid(),
              container.getNamespaces(),
              uid,
              gid,
              supplementaryGids,
              capsToKeep,
              workDirInContainer,
              envToKeep,
              envToSet)
{
    // Set TTY
    if (ttyPath.empty()) {
        mConfig.ttyFD = -1;
    } else {
        mConfig.ttyFD = utils::open(ttyPath, O_RDWR | O_NOCTTY);
    }
}

Attach::~Attach()
{
    utils::close(mConfig.ttyFD);
}

void Attach::execute()
{
    // Channels for passing configuration and synchronization
    const std::string mIntermChannelFDStr = std::to_string(mIntermChannel.getRightFD());

    const pid_t interPid = lxcpp::fork();
    if (interPid > 0) {
        mIntermChannel.setLeft();

        parent(interPid);

    } else {
        mIntermChannel.setRight();

        const char *argv[] = {ATTACH_PATH,
                              mIntermChannelFDStr.c_str(),
                              NULL
                             };
        ::execve(argv[0], const_cast<char *const*>(argv), nullptr);
        ::_exit(EXIT_FAILURE);
    }
}

void Attach::parent(const pid_t interPid)
{
    // TODO: Setup cgroups etc
    const pid_t childPid = mIntermChannel.read<pid_t>();

    // Wait for all processes
    lxcpp::waitpid(interPid);
    lxcpp::waitpid(childPid);
}

} // namespace lxcpp
