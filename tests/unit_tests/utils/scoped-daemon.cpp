/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Piotr Bartosiewicz <p.bartosiewi@partner.samsung.com>
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License
 */

/**
 * @file
 * @author  Piotr Bartosiewicz (p.bartosiewi@partner.samsung.com)
 * @brief   Starts external daemon in constructor, stops it in destructor
 */

#include "config.hpp"

#include "utils/scoped-daemon.hpp"

#include "log/logger.hpp"

#include <unistd.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#include <stdexcept>


namespace security_containers {
namespace utils {


/*
 * Scoped Daemon - sequence diagram.
 *
 *
 *                |(main process)
 *                |
 *   constructor  |
 *   ------------>|_______
 *                |       |(launcher process)
 *                |       |_______
 *                |       |       |(daemon process)
 *                |       |       |
 *                |       |       |
 *   destructor   |       |       |
 *   ------------>|  sig  |       |
 *                |------>|  sig  |
 *                |       |------>|
 *                |       |_______|
 *                |_______|
 *   destructor   |
 *      ends      |
 *
 *
 * Launcher helper process is used to monitor main process.
 * When e.g. it crashes or hits an assert then launcher kills daemon and itself.
 */

namespace {

volatile pid_t daemonPid = -1;// available in launcher process only;

void startDaemon(const char* path, const char* const argv[])
{
    execv(path, const_cast<char* const*>(argv));
    perror("exec failed");
}

void waitForDaemon()
{
    if (waitpid(daemonPid, NULL, 0) == -1) {
        perror("wait for daemon failed");
    }
}

void launcherSignalHandler(int sig)
{
    // forward to daemon
    if (kill(daemonPid, sig) == -1) {
        perror("kill daemon failed");
    }
}

void registerLauncherSignalHandler()
{
    signal(SIGTERM, launcherSignalHandler);
}

void registerParentDiedNotification()
{
    prctl(PR_SET_PDEATHSIG, SIGTERM);
}

void cleanupProcess()
{
    signal(SIGCHLD, SIG_DFL);
    signal(SIGINT, SIG_DFL);
    signal(SIGTERM, SIG_DFL);
    signal(SIGHUP, SIG_DFL);
}

void startByLauncher(const char* path, const char* const argv[])
{
    cleanupProcess();
    daemonPid = fork();
    if (daemonPid == -1) {
        perror("fork failed");
        return;
    }
    if (daemonPid == 0) {
        startDaemon(path, argv);
        _exit(1);
    }
    registerLauncherSignalHandler();
    registerParentDiedNotification();
    waitForDaemon();
}

} // namespace

ScopedDaemon::ScopedDaemon()
    : mPid(-1)
{
}

ScopedDaemon::~ScopedDaemon()
{
    stop();
}

void ScopedDaemon::start(const char* path, const char* const argv[], const bool useLauncher)
{
    if (mPid != -1) {
        throw std::runtime_error("already started");
    }
    mPid = fork();
    if (mPid == -1) {
        throw std::runtime_error("fork failed");
    }
    if (mPid == 0) {
        if (useLauncher) {
            startByLauncher(path, argv);
        } else {
            startDaemon(path, argv);
        }
        _exit(0);
    }
}

void ScopedDaemon::stop()
{
    if (mPid == -1) {
        return;
    }
    if (kill(mPid, SIGTERM) == -1) {
        LOGE("kill failed");
    }
    if (waitpid(mPid, NULL, 0) == -1) {
        LOGE("waitpid failed");
    }
    mPid = -1;
}


} // namespace utils
} // namespace security_containers
