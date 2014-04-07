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

#include "server.hpp"
#include "containers-manager.hpp"
#include "exception.hpp"

#include "log/logger.hpp"
#include "utils/glib-loop.hpp"

#include <csignal>
#include <string>

namespace security_containers {


Server::Server(const std::string& configPath)
    : mConfigPath(configPath)
{
}


Server::~Server()
{
}


namespace {
utils::Latch signalLatch;
void signalHandler(const int sig)
{
    LOGI("Got signal " << sig);
    signalLatch.set();
}
}

void Server::run()
{
    signal(SIGINT,  signalHandler);
    signal(SIGTERM, signalHandler);

    LOGI("Starting daemon...");
    {
        utils::ScopedGlibLoop loop;
        ContainersManager manager(mConfigPath);

        manager.startAll();
        LOGI("Daemon started");

        signalLatch.wait();

        LOGI("Stopping daemon...");
        // manager.stopAll() will be called in destructor
    }
    LOGI("Daemon stopped");
}

void Server::terminate()
{
    LOGI("Terminating server");
    signalLatch.set();
}

} // namespace security_containers
