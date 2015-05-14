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
#include "utils/execute.hpp"

#include "logger/logger.hpp"

#include <unistd.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#include <stdexcept>


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

bool startDaemon(const char* path, const char* const argv[])
{
    execv(path, const_cast<char* const*>(argv));
    perror("exec failed");
    return false;
}

bool waitForDaemon()
{
    int status;
    return waitPid(daemonPid, status);
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

bool startByLauncher(const char* path, const char* const argv[])
{
    cleanupProcess();
    daemonPid = fork();
    if (daemonPid == -1) {
        perror("fork failed");
        return false;
    }
    if (daemonPid == 0) {
        if (!startDaemon(path, argv)) {
            return false;
        }
    }
    registerLauncherSignalHandler();
    registerParentDiedNotification();
    return waitForDaemon();
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
        bool ret;
        if (useLauncher) {
            ret = startByLauncher(path, argv);
        } else {
            ret = startDaemon(path, argv);
        }
        _exit(ret ? EXIT_SUCCESS : EXIT_FAILURE);
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
    int status;
    if (!waitPid(mPid, status)) {
        throw std::runtime_error("waitpid failed");
    }
    if (status != EXIT_SUCCESS) {
        LOGW("process exit with status " << status);
    }
    mPid = -1;
}


} // namespace utils
