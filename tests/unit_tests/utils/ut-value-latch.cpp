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
 * @brief   Unit tests of ValueLatch interface
 */

#include "config.hpp"

#include "ut.hpp"

#include "utils/value-latch.hpp"

#include <thread>
#include <string>

BOOST_AUTO_TEST_SUITE(ValueLatchSuite)

using namespace utils;

namespace
{
    const int TIMEOUT = 1000; // ms
    const int EXPECTED_TIMEOUT = 200; // ms
    const std::string TEST_STRING = "some_random text";

    struct ComplexType
    {
        float value;
        std::string str;
    };

    struct ComplexMovableType
    {
        explicit ComplexMovableType(const ComplexType& val)
            : value(val) {};
        ComplexMovableType(const ComplexMovableType&) = delete;
        ComplexMovableType(ComplexMovableType&&) = default;

        ComplexType value;
    };
} // namespace

BOOST_AUTO_TEST_CASE(SimpleValue)
{
    ValueLatch<int> testLatch;

    std::thread testThread([&testLatch]() {
        testLatch.set(3);
    });

    testThread.join();

    BOOST_REQUIRE_EQUAL(testLatch.get(TIMEOUT), 3);
}

BOOST_AUTO_TEST_CASE(ComplexValue)
{
    ValueLatch<ComplexType> testLatch;

    std::thread testThread([&testLatch]() {
        testLatch.set({ 2.5f, TEST_STRING });
    });

    testThread.join();

    ComplexType test(testLatch.get(TIMEOUT));
    BOOST_REQUIRE_EQUAL(test.value, 2.5f);
    BOOST_REQUIRE_EQUAL(test.str, TEST_STRING);
}

BOOST_AUTO_TEST_CASE(ComplexMovableValue)
{
    ValueLatch<ComplexMovableType> testLatch;

    std::thread testThread([&testLatch]() {
        testLatch.set( ComplexMovableType({ 2.5f, TEST_STRING }) );
    });

    testThread.join();

    ComplexMovableType test(testLatch.get(TIMEOUT));
    BOOST_REQUIRE_EQUAL(test.value.value, 2.5f);
    BOOST_REQUIRE_EQUAL(test.value.str, TEST_STRING);
}

BOOST_AUTO_TEST_CASE(Timeout)
{
    ValueLatch<int> testLatch;

    BOOST_REQUIRE_EXCEPTION(testLatch.get(EXPECTED_TIMEOUT),
                            UtilsException,
                            WhatEquals("Timeout occured"));
}

BOOST_AUTO_TEST_CASE(MultipleSet)
{
    ValueLatch<int> testLatch;

    testLatch.set(3);
    BOOST_REQUIRE_EXCEPTION(testLatch.set(2),
                            UtilsException,
                            WhatEquals("Cannot set value multiple times"));
}

BOOST_AUTO_TEST_CASE(MultipleGet)
{
    ValueLatch<int> testLatch;

    testLatch.set(3);
    testLatch.get(TIMEOUT);
    BOOST_REQUIRE_EXCEPTION(testLatch.get(EXPECTED_TIMEOUT),
                            UtilsException,
                            WhatEquals("Timeout occured"));
}

BOOST_AUTO_TEST_SUITE_END()
