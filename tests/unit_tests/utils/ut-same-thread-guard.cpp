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
 * @brief   Unit tests of same thread guard
 */

#include "config.hpp"

#include "ut.hpp"

#include "utils/same-thread-guard.hpp"
#include <thread>

#ifdef ENABLE_SAME_THREAD_GUARD

BOOST_AUTO_TEST_SUITE(SameThreadGuardSuite)

using namespace utils;

BOOST_AUTO_TEST_CASE(Simple)
{
    SameThreadGuard guard;
    BOOST_CHECK(guard.check());
    BOOST_CHECK(guard.check());
    guard.reset();
    BOOST_CHECK(guard.check());
    BOOST_CHECK(guard.check());
}

BOOST_AUTO_TEST_CASE(Thread)
{
    SameThreadGuard guard;

    std::thread([&] {
        BOOST_CHECK(guard.check());
    }).join();

    BOOST_CHECK(!guard.check());
    BOOST_CHECK(!guard.check());

    guard.reset();
    BOOST_CHECK(guard.check());

    std::thread([&] {
        BOOST_CHECK(!guard.check());
    }).join();
}

BOOST_AUTO_TEST_SUITE_END()

#endif // ENABLE_SAME_THREAD_GUARD
