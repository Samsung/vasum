/*
 *  Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Krzysztof Dynowski (k.dynowski@samsumg.com)
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
 * @author  Krzysztof Dynowski (k.dynowski@samsung.com)
 * @brief   Unit tests of c-like args builder
 */
#include "config.hpp"
#include "ut.hpp"

#include "utils/c-args.hpp"

BOOST_AUTO_TEST_SUITE(UtilsCArgsSuite)

using namespace utils;

BOOST_AUTO_TEST_CASE(CArgsBuilderTest1)
{
    CArgsBuilder args;

    {
        std::string vx = std::to_string(20);
        args.add(std::to_string(10))
            .add(vx);
    }
    BOOST_CHECK(std::string("10") == args[0]);
    BOOST_CHECK(std::string("20") == args[1]);

    {
        std::string vx = std::to_string(22);
        args.add(std::to_string(12))
            .add(vx);
    }
    BOOST_CHECK(std::string("10") == args[0]);
    BOOST_CHECK(std::string("20") == args[1]);
    BOOST_CHECK(std::string("12") == args[2]);
    BOOST_CHECK(std::string("22") == args[3]);
}

BOOST_AUTO_TEST_CASE(CArgsBuilderTest2)
{
    CArgsBuilder args;
    for (int i = 0; i < 10; ++i) {
        args.add(i + 10);
    }

    for (int i = 0; i < 10; ++i) {
        int t = std::stoi(args[i]);
        BOOST_CHECK(t == i + 10);
    }

    const char * const * c_array = args.c_array();
    for (int i = 0; i < 10; ++i) {
        int t = std::stoi(std::string(c_array[i]));
        BOOST_CHECK(t == i + 10);
    }
}

BOOST_AUTO_TEST_SUITE_END()
