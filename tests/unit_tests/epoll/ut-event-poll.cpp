/*
 *  Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
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
 * @brief   Unit tests of event poll
 */

#include "config.hpp"
#include "ut.hpp"

#include "epoll/event-poll.hpp"
#include "logger/logger.hpp"
#include "ipc/internals/socket.hpp"
#include "utils/latch.hpp"
#include "utils/glib-loop.hpp"
#include "epoll/glib-dispatcher.hpp"
#include "epoll/thread-dispatcher.hpp"

using namespace vasum::utils;
using namespace vasum::epoll;
using namespace vasum::ipc;

namespace {

const int unsigned TIMEOUT = 1000;

} // namespace

BOOST_AUTO_TEST_SUITE(EventPollSuite)

BOOST_AUTO_TEST_CASE(EmptyPoll)
{
    EventPoll poll;
    BOOST_CHECK(!poll.dispatchIteration(0));
}

BOOST_AUTO_TEST_CASE(ThreadedPoll)
{
    ThreadDispatcher dispatcher;
}

BOOST_AUTO_TEST_CASE(GlibPoll)
{
    ScopedGlibLoop loop;

    GlibDispatcher dispatcher;
}

void doSocketTest(EventPoll& poll)
{
    const std::string PATH = "/tmp/ut-poll.sock";
    const std::string MESSAGE = "This is a test message";

    Latch goodMessage;
    Latch remoteClosed;

    Socket listen = Socket::createSocket(PATH);
    std::shared_ptr<Socket> server;

    auto serverCallback = [&](int, Events events) -> bool {
        LOGD("Server events: " << eventsToString(events));

        if (events & EPOLLOUT) {
            server->write(MESSAGE.data(), MESSAGE.size());
            poll.removeFD(server->getFD());
            server.reset();
        }
        return true;
    };

    auto listenCallback = [&](int, Events events) -> bool {
        LOGD("Listen events: " << eventsToString(events));
        if (events & EPOLLIN) {
            server = listen.accept();
            poll.addFD(server->getFD(), EPOLLHUP | EPOLLRDHUP | EPOLLOUT, serverCallback);
        }
        return true;
    };

    poll.addFD(listen.getFD(), EPOLLIN, listenCallback);

    Socket client = Socket::connectSocket(PATH);

    auto clientCallback = [&](int, Events events) -> bool {
        LOGD("Client events: " << eventsToString(events));

        if (events & EPOLLIN) {
            std::string ret(MESSAGE.size(), 'x');
            client.read(&ret.front(), ret.size());
            if (ret == MESSAGE) {
                goodMessage.set();
            }
        }
        if (events & EPOLLRDHUP) {
            poll.removeFD(client.getFD());
            remoteClosed.set();
        }
        return true;
    };

    poll.addFD(client.getFD(), EPOLLHUP | EPOLLRDHUP | EPOLLIN, clientCallback);

    BOOST_CHECK(goodMessage.wait(TIMEOUT));
    BOOST_CHECK(remoteClosed.wait(TIMEOUT));

    poll.removeFD(listen.getFD());
}

BOOST_AUTO_TEST_CASE(ThreadedPollSocket)
{
    ThreadDispatcher dispatcher;

    doSocketTest(dispatcher.getPoll());
}

BOOST_AUTO_TEST_CASE(GlibPollSocket)
{
    ScopedGlibLoop loop;

    GlibDispatcher dispatcher;

    doSocketTest(dispatcher.getPoll());
}

BOOST_AUTO_TEST_CASE(PollStacking)
{
    ThreadDispatcher dispatcher;

    EventPoll innerPoll;

    auto dispatchInner = [&](int, Events) -> bool {
        innerPoll.dispatchIteration(0);
        return true;
    };
    dispatcher.getPoll().addFD(innerPoll.getPollFD(), EPOLLIN, dispatchInner);
    doSocketTest(innerPoll);
    dispatcher.getPoll().removeFD(innerPoll.getPollFD());
}

BOOST_AUTO_TEST_SUITE_END()

