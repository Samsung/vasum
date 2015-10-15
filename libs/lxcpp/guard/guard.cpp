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
#include "lxcpp/commands/prep-guest-terminal.hpp"
#include "lxcpp/commands/provision.hpp"

#include "config/manager.hpp"
#include "logger/logger.hpp"

#include <unistd.h>
#include <sys/wait.h>

namespace lxcpp {

namespace {

int startContainer(void* data)
{
    ContainerConfig& config = *static_cast<ContainerConfig*>(data);

    // TODO: container preparation part 2

    Provisions provisions(config);
    provisions.execute();

    PrepGuestTerminal terminals(config.mTerminals);
    terminals.execute();

    lxcpp::execve(config.mInit);

    return EXIT_FAILURE;
}

} // namespace


Guard::Guard(const int channelFD)
    : mChannel(channelFD)
{
    mChannel.setCloseOnExec(true);
    config::loadFromFD(mChannel.getFD(), mConfig);

    logger::setupLogger(mConfig.mLogger.getType(),
                        mConfig.mLogger.getLevel(),
                        mConfig.mLogger.getArg());

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
    // TODO: container preparation part 1

    const pid_t initPid = lxcpp::clone(startContainer,
                                       &mConfig,
                                       mConfig.mNamespaces);

    mConfig.mGuardPid = ::getpid();
    mConfig.mInitPid = initPid;

    mChannel.write(mConfig.mGuardPid);
    mChannel.write(mConfig.mInitPid);
    mChannel.shutdown();

    int status = lxcpp::waitpid(initPid);
    LOGD("Init exited with status: " << status);

    // TODO: cleanup after child exits
    Provisions provisions(mConfig);
    provisions.revert();

    return status;
}

} // namespace lxcpp
