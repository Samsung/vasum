/*
 *  Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
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
 * @author  Jan Olszak (j.olszak@samsung.com)
 * @brief   Unit tests of the channel class
 */

#include "config.hpp"

#include "ut.hpp"

#include "utils/channel.hpp"
#include "utils/execute.hpp"

BOOST_AUTO_TEST_SUITE(ChannelSuite)

using namespace utils;

BOOST_AUTO_TEST_CASE(ConstructorDestructor)
{
    Channel c;
}

BOOST_AUTO_TEST_CASE(SetLeftRight)
{
    const int TEST_PASSED = 0;
    const int ERROR = 1;
    const int DATA = 1234;

    Channel c;

    pid_t pid = ::fork();
    if (pid == -1) {
        BOOST_REQUIRE(false);
    }

    if (pid == 0) {
        try {
            c.setLeft();
            c.write(DATA);
            c.shutdown();
            ::_exit(TEST_PASSED);
        } catch(...) {
            ::_exit(ERROR);
        }
    }

    c.setRight();

    int recData = c.read<int>();

    BOOST_REQUIRE(recData == DATA);

    int status = -1;
    BOOST_REQUIRE(utils::waitPid(pid, status));
    BOOST_REQUIRE(status == TEST_PASSED);
    c.shutdown();

}

BOOST_AUTO_TEST_SUITE_END()
