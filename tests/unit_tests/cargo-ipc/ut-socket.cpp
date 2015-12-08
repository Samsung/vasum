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
 * @brief   Socket IPC module tests
 */

#include "config.hpp"

#include "ut.hpp"
#include "logger/logger.hpp"
#include "cargo-ipc/internals/socket.hpp"

#include <thread>

using namespace cargo::ipc;
using namespace cargo::ipc::internals;

const std::string SOCKET_PATH = "/tmp/test.socket";

BOOST_AUTO_TEST_SUITE(SocketSuite)

#ifdef HAVE_SYSTEMD
#include "socket-test.hpp"

BOOST_AUTO_TEST_CASE(SystemdSocket)
{
    std::string readMessage;

    {
        Socket socket = Socket::connectUNIX(vasum::socket_test::SOCKET_PATH);
        BOOST_REQUIRE_GT(socket.getFD(), -1);

        readMessage.resize(vasum::socket_test::TEST_MESSAGE.size());
        socket.read(&readMessage.front(), readMessage.size());
    }

    BOOST_REQUIRE_EQUAL(readMessage, vasum::socket_test::TEST_MESSAGE);
}
#endif // HAVE_SYSTEMD

BOOST_AUTO_TEST_CASE(GetSocketType)
{
    {
        Socket socket;
        BOOST_CHECK(socket.getType() == Socket::Type::INVALID);
    }

    {
        Socket socket = Socket::createINET("localhost", "");
        BOOST_CHECK(socket.getType() == Socket::Type::INET);
    }

    {
        Socket socket = Socket::createUNIX(SOCKET_PATH);
        BOOST_CHECK(socket.getType() == Socket::Type::UNIX);
    }
}

BOOST_AUTO_TEST_CASE(InternetSocket)
{
    const char msg[] = "MESSAGE";
    const std::string host = "127.0.0.1";

    Socket server = Socket::createINET(host, "");
    const unsigned short port = server.getPort();

    BOOST_CHECK(server.getType() == Socket::Type::INET);

    auto clientThread = std::thread([&] {
        Socket client = Socket::connectINET(host, std::to_string(port));
        BOOST_CHECK(client.getType() == Socket::Type::INET);
        client.write(msg, sizeof(msg));

        char buffer[sizeof(msg)];
        client.read(buffer, sizeof(msg));
        BOOST_CHECK_EQUAL(buffer, msg);
    });

    auto connection = server.accept();
    char buffer[sizeof(msg)];

    connection->read(buffer, sizeof(msg));
    BOOST_CHECK_EQUAL(buffer, msg);

    connection->write(msg, sizeof(msg));

    clientThread.join();
}

BOOST_AUTO_TEST_SUITE_END()
