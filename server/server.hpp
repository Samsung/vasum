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
 * @brief   Server class declaration
 */


#ifndef SERVER_SERVER_HPP
#define SERVER_SERVER_HPP

#include "zones-manager.hpp"

#include "utils/latch.hpp"
#include "utils/signalfd.hpp"
#include "utils/glib-loop.hpp"
#include "cargo-ipc/epoll/event-poll.hpp"

#include <atomic>
#include <string>
#include <pthread.h>


namespace vasum {


class Server {
public:
    Server(const std::string& configPath);

    /**
     * Starts all the zones and blocks until SIGINT, SIGTERM or SIGUSR1
     */
    void run(bool asRoot);

    /**
     * Reload the server by launching execve on itself if SIGUSR1 was sent to server.
     */
    void reloadIfRequired(char* argv[]);

    /**
     * Terminates the server.
     * Equivalent of sending SIGINT or SIGTERM signal
     */
    void terminate();

    /**
     * Check server runtime environment
    */
    static bool checkEnvironment();

private:
    bool mIsRunning;
    bool mIsUpdate;
    std::string mConfigPath;
    utils::ScopedGlibLoop loop;
    cargo::ipc::epoll::EventPoll mEventPoll;
    utils::SignalFD mSignalFD;
    ZonesManager mZonesManager;
    ::pthread_t mDispatchingThread;

    /**
     * Set needed caps, groups and drop root privileges.
     */
    static bool prepareEnvironment(const std::string& configPath, bool runAsRoot);

    void handleUpdate();
    void handleStop();

};


} // namespace vasum


#endif // SERVER_SERVER_HPP
