/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
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
 * @brief   Unit tests of callback guard
 */

#include "config.hpp"

#include "ut.hpp"

#include "utils/callback-guard.hpp"
#include "utils/latch.hpp"

#include <future>
#include <thread>

BOOST_AUTO_TEST_SUITE(CallbackGuardSuite)

using namespace utils;

const int unsigned TIMEOUT = 1000;

BOOST_AUTO_TEST_CASE(Empty)
{
    CallbackGuard guard;
    BOOST_CHECK_EQUAL(0, guard.getTrackersCount());
    BOOST_CHECK(guard.waitForTrackers(TIMEOUT));
}

BOOST_AUTO_TEST_CASE(Simple)
{
    CallbackGuard guard;
    guard.spawn();
    guard.spawn();
    BOOST_CHECK_EQUAL(0, guard.getTrackersCount());
    CallbackGuard::Tracker tracker1 = guard.spawn();
    CallbackGuard::Tracker tracker2 = guard.spawn();
    BOOST_CHECK_EQUAL(2, guard.getTrackersCount());
    CallbackGuard::Tracker tracker2Copy = tracker2;
    BOOST_CHECK_EQUAL(2, guard.getTrackersCount());
    tracker2.reset();
    BOOST_CHECK_EQUAL(2, guard.getTrackersCount());
    tracker2Copy.reset();
    BOOST_CHECK_EQUAL(1, guard.getTrackersCount());
    tracker1.reset();
    BOOST_CHECK_EQUAL(0, guard.getTrackersCount());
    BOOST_CHECK(guard.waitForTrackers(TIMEOUT));
}

BOOST_AUTO_TEST_CASE(Thread)
{
    Latch trackerCreated;
    Latch trackerCanBeDestroyed;
    CallbackGuard guard;

    std::future<bool> future = std::async(std::launch::async, [&]() -> bool {
            CallbackGuard::Tracker tracker = guard.spawn();
            trackerCreated.set();
            if (!trackerCanBeDestroyed.wait(TIMEOUT)) {
                return false;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            return true;
        });

    BOOST_CHECK(trackerCreated.wait(TIMEOUT));
    BOOST_CHECK_EQUAL(1, guard.getTrackersCount());

    trackerCanBeDestroyed.set();
    BOOST_CHECK(guard.waitForTrackers(TIMEOUT));
    BOOST_CHECK_EQUAL(0, guard.getTrackersCount());

    future.wait();
    BOOST_CHECK(future.get());
}

BOOST_AUTO_TEST_SUITE_END()
