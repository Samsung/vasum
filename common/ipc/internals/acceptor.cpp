/*
*  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
*
*  Contact: Jan Olszak <j.olszak@samsung.com>
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
 * @brief   Class for accepting new connections
 */

#include "config.hpp"

#include "ipc/exception.hpp"
#include "ipc/internals/utils.hpp"
#include "ipc/internals/acceptor.hpp"
#include "logger/logger.hpp"

#include <poll.h>
#include <cerrno>
#include <cstring>
#include <chrono>
#include <vector>

namespace security_containers {
namespace ipc {

Acceptor::Acceptor(const std::string& socketPath, const NewConnectionCallback& newConnectionCallback)
    : mNewConnectionCallback(newConnectionCallback),
      mSocket(Socket::createSocket(socketPath))
{
    LOGT("Creating Acceptor for socket " << socketPath);
}

Acceptor::~Acceptor()
{
    LOGT("Destroying Acceptor");
    try {
        stop();
    } catch (IPCException& e) {
        LOGE("Error in destructor: " << e.what());
    }
    LOGT("Destroyed Acceptor");
}

void Acceptor::start()
{
    LOGT("Starting Acceptor");
    if (!mThread.joinable()) {
        mThread = std::thread(&Acceptor::run, this);
    }
    LOGT("Started Acceptor");
}

void Acceptor::stop()
{
    LOGT("Stopping Acceptor");
    if (mThread.joinable()) {
        LOGT("Event::FINISH -> Acceptor");
        mEventQueue.send(Event::FINISH);
        LOGT("Waiting for Acceptor to finish");
        mThread.join();
    }
    LOGT("Stopped Acceptor");
}

void Acceptor::run()
{
    // Setup polling structure
    std::vector<struct pollfd> fds(2);

    fds[0].fd = mEventQueue.getFD();
    fds[0].events = POLLIN;

    fds[1].fd = mSocket.getFD();
    fds[1].events = POLLIN;

    // Main loop
    bool isRunning = true;
    while (isRunning) {
        LOGT("Waiting for new connections...");

        int ret = ::poll(fds.data(), fds.size(), -1 /*blocking call*/);

        LOGT("...Incoming connection!");

        if (ret == -1 || ret == 0) {
            if (errno == EINTR) {
                continue;
            }
            LOGE("Error in poll: " << std::string(strerror(errno)));
            throw IPCException("Error in poll: " + std::string(strerror(errno)));
            break;
        }

        // Check for incoming connections
        if (fds[1].revents & POLLIN) {
            fds[1].revents = 0;
            std::shared_ptr<Socket> tmpSocket = mSocket.accept();
            mNewConnectionCallback(tmpSocket);
        }

        // Check for incoming events
        if (fds[0].revents & POLLIN) {
            fds[0].revents = 0;

            if (mEventQueue.receive() == Event::FINISH) {
                LOGD("Event FINISH");
                isRunning = false;
                break;
            }
        }
    }
    LOGT("Exiting run");
}

} // namespace ipc
} // namespace security_containers
