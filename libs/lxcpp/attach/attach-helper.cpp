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

#include "lxcpp/attach/attach-helper.hpp"
#include "lxcpp/exception.hpp"
#include "lxcpp/process.hpp"
#include "lxcpp/filesystem.hpp"
#include "lxcpp/namespace.hpp"
#include "lxcpp/capability.hpp"
#include "lxcpp/environment.hpp"
#include "lxcpp/credentials.hpp"
#include "lxcpp/utils.hpp"

#include "utils/exception.hpp"
#include "utils/fd-utils.hpp"
#include "logger/logger.hpp"
#include "cargo/manager.hpp"

#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>

#include <string>

namespace lxcpp {

namespace {

int child(void* data)
{
    AttachConfig& config = *static_cast<AttachConfig*>(data);

    // Setup /proc /sys mount
    setupMountPoints();

    // Setup capabilities
    dropCapsFromBoundingExcept(config.capsToKeep);

    // Setup environment variables
    clearenvExcept(config.envToKeep);
    setenv(config.envToSet);

    // Set uid/gids
    lxcpp::setregid(config.gid, config.gid);
    setgroups(config.supplementaryGids);
    lxcpp::setreuid(config.uid, config.uid);

    // Set control TTY
    if(!setupControlTTY(config.ttyFD)) {
        ::_exit(EXIT_FAILURE);
    }

    lxcpp::execve(config.argv);

    return EXIT_FAILURE;
}

} // namespace

AttachHelper::AttachHelper(const int channelFD)
    : mChannel(channelFD)
{
    mChannel.setCloseOnExec(true);
    cargo::loadFromFD(mChannel.getFD(), mConfig);
    logger::setupLogger(mConfig.logger.mType,
                        mConfig.logger.mLevel,
                        mConfig.logger.mArg);
    LOGD("Config & logging restored");
}

AttachHelper::~AttachHelper()
{
    utils::close(mConfig.ttyFD);
}

void AttachHelper::execute()
{
    // Move to the right namespaces
    lxcpp::setns(mConfig.initPid, mConfig.namespaces);

    // Change the current work directory
    // workDirInContainer is a path relative to the container's root
    lxcpp::chdir(mConfig.workDirInContainer);

    // Unsharing PID namespace won't affect the returned childPid
    // CLONE_PARENT: Child's PPID == Caller's PID
    const pid_t childPid = lxcpp::clone(child,
                                        &mConfig,
                                        CLONE_PARENT);
    mChannel.write(childPid);
}

} // namespace lxcpp
