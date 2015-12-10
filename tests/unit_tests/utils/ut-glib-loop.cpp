/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Lukasz Pawelczyk <l.pawelczyk@partner.samsung.com>
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
 * @author  Lukasz Pawelczyk (l.pawelczyk@partner.samsung.com)
 * @brief   Unit tests of glib-loop
 */

#include "config.hpp"

#include "ut.hpp"

#include "utils/glib-loop.hpp"

#include <atomic>

BOOST_AUTO_TEST_SUITE(GlibLoopSuite)

using namespace utils;


namespace {

const unsigned int TIMER_INTERVAL_MS = 100;
const unsigned int TIMER_NUMBER      = 4;
const unsigned int TIMER_WAIT_FOR    = 2 * TIMER_NUMBER * TIMER_INTERVAL_MS;

} // namespace

BOOST_AUTO_TEST_CASE(GlibLoopTest)
{
    ScopedGlibLoop loop;
}

BOOST_AUTO_TEST_CASE(GlibTimerEvent)
{
    ScopedGlibLoop loop;
    std::atomic_uint counter(0);

    CallbackGuard guard;

    auto callback = [&]()-> bool {
        if (++counter >= TIMER_NUMBER) {
            return false;
        }
        return true;
    };

    Glib::addTimerEvent(TIMER_INTERVAL_MS, callback, guard);

    BOOST_CHECK(counter < TIMER_NUMBER);
    BOOST_CHECK(guard.waitForTrackers(TIMER_WAIT_FOR));
    BOOST_CHECK_EQUAL(counter, TIMER_NUMBER);
}

BOOST_AUTO_TEST_SUITE_END()
