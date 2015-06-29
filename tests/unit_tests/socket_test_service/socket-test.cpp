/*
 *  Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Lukasz Kostyra <l.kostyra@samsung.com>
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
 * @author  Lukasz Kostyra (l.kostyra@samsung.com)
 * @brief   Mini-service for IPC Socket mechanism tests
 */

#include "config.hpp"
#include "socket-test.hpp"
#include "logger/logger.hpp"
#include "logger/backend-journal.hpp"
#include "ipc/internals/socket.hpp"
#include "ipc/exception.hpp"

#include <cstring>
#include <memory>

using namespace vasum::socket_test;
using namespace ipc;
using namespace logger;

// NOTE this is a single-usage program, only meant to test vasum::ipc::Socket module.
//      It's purpose is to be activated when needed by systemd socket activation mechanism.
int main()
{
    Logger::setLogLevel(LogLevel::TRACE);
    Logger::setLogBackend(new SystemdJournalBackend());

    try {
        Socket listeningSocket(Socket::createSocket(SOCKET_PATH));
        if (listeningSocket.getFD() < 0) {
            LOGE("Failed to connect to socket!");
            return 1;
        }

        std::shared_ptr<Socket> clientSocket = listeningSocket.accept();
        LOGI("Connected! Emitting message to client.");
        clientSocket->write(TEST_MESSAGE.c_str(), TEST_MESSAGE.size());
        LOGI("Message sent through socket! Exiting.");
    } catch (const IPCException& e) {
        LOGE("IPC exception caught! " << e.what());
        return 1;
    }

    return 0;
}
