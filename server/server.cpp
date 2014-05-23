/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Jan Olszak (j.olszak@samsung.com)
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
 * @author  Jan Olszak (j.olszak@samsung.com)
 * @brief   Server class definition
 */

#include "config.hpp"

#include "server.hpp"
#include "containers-manager.hpp"
#include "exception.hpp"

#include "log/logger.hpp"
#include "utils/glib-loop.hpp"

#include <csignal>
#include <cerrno>
#include <string>
#include <cstring>
#include <atomic>
#include <unistd.h>

extern char** environ;

namespace security_containers {


Server::Server(const std::string& configPath)
    : mConfigPath(configPath)
{
}


Server::~Server()
{
}


namespace {

std::atomic_bool gUpdateTriggered(false);
utils::Latch gSignalLatch;

void signalHandler(const int sig)
{
    LOGI("Got signal " << sig);

    if (sig == SIGUSR1) {
        LOGD("Received SIGUSR1 - triggering update.");
        gUpdateTriggered = true;
    }

    gSignalLatch.set();
}

} // namespace

void Server::run()
{
    signal(SIGINT,  signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGUSR1, signalHandler);

    LOGI("Starting daemon...");
    {
        utils::ScopedGlibLoop loop;
        ContainersManager manager(mConfigPath);

        manager.startAll();
        LOGI("Daemon started");

        gSignalLatch.wait();

        // Detach containers if we triggered an update
        if (gUpdateTriggered) {
            manager.setContainersDetachOnExit();
        }

        LOGI("Stopping daemon...");
        // manager.stopAll() will be called in destructor
    }
    LOGI("Daemon stopped");
}

void Server::reloadIfRequired(char* argv[])
{
    if (gUpdateTriggered) {
        execve(argv[0], argv, environ);

        LOGE("Failed to reload " << argv[0] << ": " << strerror(errno));
    }
}

void Server::terminate()
{
    LOGI("Terminating server");
    gSignalLatch.set();
}

} // namespace security_containers
