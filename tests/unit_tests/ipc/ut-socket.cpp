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

#include "ipc/internals/socket.hpp"

#include <socket-test.hpp>

using namespace vasum::ipc;

BOOST_AUTO_TEST_SUITE(SocketSuite)

BOOST_AUTO_TEST_CASE(SystemdSocket)
{
    std::string readMessage;

    {
        Socket socket = Socket::connectSocket(vasum::socket_test::SOCKET_PATH);
        BOOST_REQUIRE_GT(socket.getFD(), -1);

        readMessage.resize(vasum::socket_test::TEST_MESSAGE.size());
        socket.read(&readMessage.front(), readMessage.size());
    }

    BOOST_REQUIRE_EQUAL(readMessage, vasum::socket_test::TEST_MESSAGE);
}

BOOST_AUTO_TEST_SUITE_END()
