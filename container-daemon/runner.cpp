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
 * @brief   Definition of the Runner class, that manages daemon's lifetime
 */

#include "config.hpp"

#include "runner.hpp"
#include "daemon.hpp"

#include "logger/logger.hpp"
#include "utils/glib-loop.hpp"
#include "utils/latch.hpp"

#include <csignal>


namespace security_containers {
namespace container_daemon {


Runner::Runner()
{
}


Runner::~Runner()
{
}


namespace {
utils::Latch gSignalLatch;

void signalHandler(const int sig)
{
    LOGI("Got signal " << sig);
    gSignalLatch.set();
}
} // namespace


void Runner::run()
{
    signal(SIGINT,  signalHandler);
    signal(SIGTERM, signalHandler);

    LOGI("Starting Container Daemon...");
    {
        utils::ScopedGlibLoop loop;
        LOGI("Container Daemon started");

        // Connects to dbus and registers API
        container_daemon::Daemon daemon;

        gSignalLatch.wait();

        LOGI("Stopping Container Daemon...");

    }
    LOGI("Daemon stopped");
}

void Runner::terminate()
{
    LOGI("Terminating Container Daemon");
    gSignalLatch.set();
}

} // namespace container_daemon
} // namespace security_containers
