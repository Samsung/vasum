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
 * @brief   LXCPP guard process implementation
 */

#include "lxcpp/utils.hpp"
#include "lxcpp/guard/guard.hpp"
#include "lxcpp/process.hpp"
#include "lxcpp/credentials.hpp"
#include "lxcpp/commands/prep-guest-terminal.hpp"
#include "lxcpp/commands/provision.hpp"
#include "lxcpp/commands/setup-userns.hpp"

#include "config/manager.hpp"
#include "logger/logger.hpp"

#include <unistd.h>
#include <sys/wait.h>

namespace lxcpp {

int Guard::startContainer(void* data)
{
    ContainerConfig& config = static_cast<ContainerData*>(data)->mConfig;
    utils::Channel& channel = static_cast<ContainerData*>(data)->mChannel;

    // wait for continue sync from guard
    channel.setRight();
    channel.read<bool>();
    channel.shutdown();

    // TODO: container preparation part 3: things to do in the container process

    Provisions provisions(config);
    provisions.execute();

    PrepGuestTerminal terminals(config.mTerminals);
    terminals.execute();

    if (config.mUserNSConfig.mUIDMaps.size()) {
        lxcpp::setreuid(0, 0);
    }
    if (config.mUserNSConfig.mGIDMaps.size()) {
        lxcpp::setregid(0, 0);
    }

    lxcpp::execve(config.mInit);

    return EXIT_FAILURE;
}

Guard::Guard(const int channelFD)
    : mChannel(channelFD)
{
    mChannel.setCloseOnExec(true);
    config::loadFromFD(mChannel.getFD(), mConfig);

    logger::setupLogger(mConfig.mLogger.mType,
                        mConfig.mLogger.mLevel,
                        mConfig.mLogger.mArg);

    LOGD("Config & logging restored");

    try {
        LOGD("Setting the guard process title");
        const std::string title = "[LXCPP] " + mConfig.mName + " " + mConfig.mRootPath;
        setProcTitle(title);
    } catch (std::exception &e) {
        // Ignore, this is optional
        LOGW("Failed to set the guard process title: " << e.what());
    }
}

Guard::~Guard()
{
}

int Guard::execute()
{
    // TODO: container preparation part 1: things to do before clone

    utils::Channel channel;
    ContainerData data(mConfig, channel);

    const pid_t initPid = lxcpp::clone(Guard::startContainer,
                                       &data,
                                       mConfig.mNamespaces);

    mConfig.mGuardPid = ::getpid();
    mConfig.mInitPid = initPid;

    mChannel.write(mConfig.mGuardPid);
    mChannel.write(mConfig.mInitPid);
    mChannel.shutdown();

    // TODO: container preparation part 2: things to do immediately after clone

    SetupUserNS userNS(mConfig.mUserNSConfig, mConfig.mInitPid);
    userNS.execute();

    // send continue sync to container once userns, netns, cgroups, etc, are configured
    channel.setLeft();
    channel.write(true);
    channel.shutdown();

    int status = lxcpp::waitpid(initPid);
    LOGD("Init exited with status: " << status);

    // TODO: container (de)preparation part 4: cleanup after container quits

    Provisions provisions(mConfig);
    provisions.revert();

    return status;
}

} // namespace lxcpp
