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

#include "config/manager.hpp"
#include "logger/logger.hpp"

#include <unistd.h>
#include <sys/wait.h>


namespace lxcpp {


void startContainer(const ContainerConfig &cfg)
{
    lxcpp::execve(cfg.mInit);
}

int startGuard(int channelFD)
{
    ContainerConfig cfg;
    utils::Channel channel(channelFD);
    channel.setCloseOnExec(true);
    config::loadFromFD(channel.getFD(), cfg);

    logger::setupLogger(cfg.mLogger.getType(),
                        cfg.mLogger.getLevel(),
                        cfg.mLogger.getArg());

    LOGD("Guard started, config & logging restored");

    try {
        LOGD("Setting the guard process title");
        const std::string title = "[LXCPP] " + cfg.mName + " " + cfg.mRootPath;
        setProcTitle(title);
    } catch (std::exception &e) {
        // Ignore, this is optional
        LOGW("Failed to set the guard process title");
    }

    // TODO: container preparation part 1

    // TODO: switch to clone
    LOGD("Forking container's init process");
    pid_t pid = lxcpp::fork();

    if (pid == 0) {
        // TODO: container preparation part 2

        PrepGuestTerminal terminals(cfg.mTerminals);
        terminals.execute();

        startContainer(cfg);
        ::_exit(EXIT_FAILURE);
    }

    cfg.mGuardPid = ::getpid();
    cfg.mInitPid = pid;

    channel.write(cfg.mGuardPid);
    channel.write(cfg.mInitPid);
    channel.shutdown();

    int status = lxcpp::waitpid(pid);
    LOGD("Init exited with status: " << status);
    return status;
}


} // namespace lxcpp
