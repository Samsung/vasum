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
 * @brief   Implementation of starting a container
 */

#include "lxcpp/commands/start.hpp"
#include "lxcpp/exception.hpp"
#include "lxcpp/process.hpp"
#include "lxcpp/utils.hpp"
#include "lxcpp/terminal.hpp"

#include "logger/logger.hpp"
#include "config/manager.hpp"

#include <unistd.h>
#include <stdio.h>
#include <sys/ioctl.h>


namespace lxcpp {


Start::Start(ContainerConfig &config)
    : mConfig(config),
      mChannel(false),
      mGuardPath(GUARD_PATH),
      mChannelFD(std::to_string(mChannel.getRightFD()))
{
}

Start::~Start()
{
}

void Start::execute()
{
    LOGD("Forking daemonize and guard processes. Execing guard libexec binary.");
    LOGD("Logging will cease now. It should be restored using some new facility in the guard process.");
    const pid_t pid = lxcpp::fork();

    if (pid > 0) {
        mChannel.setLeft();
        parent(pid);
    } else {
        // Below this point only safe functions mentioned in signal(7) are allowed.
        mChannel.setRight();
        daemonize();
        ::_exit(EXIT_FAILURE);
    }
}

void Start::parent(const pid_t pid)
{
    int status = lxcpp::waitpid(pid);
    if (status != EXIT_SUCCESS) {
        const std::string msg = "Problem with a daemonize process";
        LOGE(msg);
        throw ProcessSetupException(msg);
    }

    // Send the config to the guard process
    config::saveToFD(mChannel.getFD(), mConfig);

    // Read the pids from the guard process
    mConfig.mGuardPid = mChannel.read<pid_t>();
    mConfig.mInitPid = mChannel.read<pid_t>();

    mChannel.shutdown();

    if (mConfig.mGuardPid <= 0 || mConfig.mInitPid <= 0) {
        const std::string msg = "Problem with receiving pids";
        LOGE(msg);
        throw ProcessSetupException(msg);
    }

    LOGD("Guard PID: " << mConfig.mGuardPid);
    LOGD("Init PID: " << mConfig.mInitPid);
}

void Start::daemonize()
{
    // Prepare a clean daemonized environment for a guard process:

    // Set a new session so the process looses its control terminal
    if (::setsid() < 0) {
        ::_exit(EXIT_FAILURE);
    }

    // Double fork() with exit() to reattach the process under the host's init
    // and to make sure that the child (guard) is not a process group leader
    // and cannot reacquire its control terminal
    pid_t pid = ::fork();
    if (pid < 0) { // fork failed
        ::_exit(EXIT_FAILURE);
    }
    if (pid > 0) { // exit in parent process
        ::_exit(EXIT_SUCCESS);
    }

    // Chdir to / so it's independent on other directories
    if (::chdir("/") < 0) {
        ::_exit(EXIT_FAILURE);
    }

    // Null std* fds so it's properly dettached from the terminal
    if (nullStdFDs() < 0) {
        ::_exit(EXIT_FAILURE);
    }

    // Add name and path of the container to argv. They are not used, but will
    // identify the container in the process list in case setProcTitle() fails
    // and will guarantee we have enough argv memory to write the title we want.
    const char *argv[] = {mGuardPath.c_str(),
                          mChannelFD.c_str(),
                          mConfig.mName.c_str(),
                          mConfig.mRootPath.c_str(),
                          NULL};
    ::execve(argv[0], const_cast<char *const*>(argv), NULL);
    ::_exit(EXIT_FAILURE);
}


} // namespace lxcpp
