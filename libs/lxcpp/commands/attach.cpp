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

#include "config.hpp"

#include "lxcpp/commands/attach.hpp"
#include "lxcpp/exception.hpp"
#include "lxcpp/process.hpp"
#include "utils/exception.hpp"
#include "utils/fd-utils.hpp"
#include "logger/logger.hpp"

#include <unistd.h>

namespace lxcpp {

Attach::Attach(const ContainerConfig& config,
               const std::vector<std::string>& argv,
               const uid_t uid,
               const gid_t gid,
               const std::string& ttyPath,
               const std::vector<gid_t>& supplementaryGids,
               const int capsToKeep,
               const std::string& workDirInContainer,
               const std::vector<std::string>& envToKeep,
               const std::vector<std::pair<std::string, std::string>>& envToSet,
               const LoggerConfig & logger)
    : mIntermChannel(false),
      mConfig(argv,
              config.mInitPid,
              config.mNamespaces,
              uid,
              gid,
              supplementaryGids,
              capsToKeep,
              workDirInContainer,
              envToKeep,
              envToSet,
              logger),
      mExitCode(EXIT_FAILURE)
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
    utils::CArgsBuilder argv;
    argv.add(ATTACH_PATH)
        .add(mIntermChannelFDStr.c_str());

    const pid_t interPid = lxcpp::fork();
    if (interPid > 0) {
        mIntermChannel.setLeft();

        parent(interPid);

    } else {
        mIntermChannel.setRight();

        lxcpp::execve(argv);
        ::_exit(EXIT_FAILURE);
    }
}

void Attach::parent(const pid_t interPid)
{
    // save the configuration
    cargo::saveToFD(mIntermChannel.getFD(), mConfig);

    // TODO: Setup cgroups etc
    const pid_t childPid = mIntermChannel.read<pid_t>();

    // Wait for all processes
    lxcpp::waitpid(interPid);
    mExitCode = lxcpp::waitpid(childPid);
}

int Attach::getExitCode() const
{
    return mExitCode;
}

} // namespace lxcpp
