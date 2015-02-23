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

#include "utils/event-poll.hpp"
#include "logger/logger.hpp"
#include "ipc/internals/socket.hpp"
#include "utils/latch.hpp"
#include "utils/glib-loop.hpp"
#include "utils/glib-poll-dispatcher.hpp"
#include "utils/thread-poll-dispatcher.hpp"

#include <map>
#include <sys/epoll.h>

using namespace vasum::utils;
using namespace vasum::ipc;

namespace {

const int unsigned TIMEOUT = 1000;
#define ADD_EVENT(e) {EPOLL##e, #e}
const std::map<EventPoll::Events, std::string> EVENT_NAMES = {
    ADD_EVENT(IN),
    ADD_EVENT(OUT),
    ADD_EVENT(ERR),
    ADD_EVENT(HUP),
    ADD_EVENT(RDHUP),
};
#undef ADD_EVENT

std::string strEvents(EventPoll::Events events)
{
    if (events == 0) {
        return "<none>";
    }
    std::ostringstream ss;
    for (const auto& p : EVENT_NAMES) {
        if (events & p.first) {
            ss << p.second << ", ";
            events &= ~p.first;
        }
    }
    if (events != 0) {
        ss << std::hex << events;
        return ss.str();
    } else {
        std::string ret = ss.str();
        ret.resize(ret.size() - 2);
        return ret;
    }
}

} // namespace

BOOST_AUTO_TEST_SUITE(EventPollSuite)

BOOST_AUTO_TEST_CASE(EmptyPoll)
{
    EventPoll poll;
    BOOST_CHECK(!poll.dispatchIteration(0));
}

BOOST_AUTO_TEST_CASE(ThreadedPoll)
{
    EventPoll poll;
    ThreadPollDispatcher dispatcher(poll);
}

BOOST_AUTO_TEST_CASE(GlibPoll)
{
    ScopedGlibLoop loop;

    EventPoll poll;
    GlibPollDispatcher dispatcher(poll);
}

void doSocketTest(EventPoll& poll, Latch& goodMessage, Latch& remoteClosed)
{
    const std::string PATH = "/tmp/ut-poll.sock";
    const std::string MESSAGE = "This is a test message";

    Socket listen = Socket::createSocket(PATH);
    std::shared_ptr<Socket> server;

    auto serverCallback = [&](int, EventPoll::Events events) -> bool {
        LOGD("Server events: " << strEvents(events));

        if (events & EPOLLOUT) {
            server->write(MESSAGE.data(), MESSAGE.size());
            poll.removeFD(server->getFD());
            server.reset();
        }
        return true;
    };

    auto listenCallback = [&](int, EventPoll::Events events) -> bool {
        LOGD("Listen events: " << strEvents(events));
        if (events & EPOLLIN) {
            server = listen.accept();
            poll.addFD(server->getFD(), EPOLLHUP | EPOLLRDHUP | EPOLLOUT, serverCallback);
        }
        return true;
    };

    poll.addFD(listen.getFD(), EPOLLIN, listenCallback);

    Socket client = Socket::connectSocket(PATH);

    auto clientCallback = [&](int, EventPoll::Events events) -> bool {
        LOGD("Client events: " << strEvents(events));

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
    Latch goodMessage;
    Latch remoteClosed;

    EventPoll poll;
    ThreadPollDispatcher dispatcher(poll);

    doSocketTest(poll, goodMessage, remoteClosed);
}

BOOST_AUTO_TEST_CASE(GlibPollSocket)
{
    Latch goodMessage;
    Latch remoteClosed;

    ScopedGlibLoop loop;

    EventPoll poll;
    GlibPollDispatcher dispatcher(poll);

    doSocketTest(poll, goodMessage, remoteClosed);
}

BOOST_AUTO_TEST_CASE(PollStacking)
{
    Latch goodMessage;
    Latch remoteClosed;

    EventPoll outer;
    EventPoll inner;

    auto dispatchInner = [&](int, EventPoll::Events) -> bool {
        inner.dispatchIteration(0);
        return true;
    };

    outer.addFD(inner.getPollFD(), EPOLLIN, dispatchInner);

    ThreadPollDispatcher dispatcher(outer);
    doSocketTest(inner, goodMessage, remoteClosed);

    outer.removeFD(inner.getPollFD());
}

BOOST_AUTO_TEST_SUITE_END()

