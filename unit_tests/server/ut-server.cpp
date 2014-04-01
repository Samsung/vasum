/*
 *  Copyright (c) 2000 - 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Bumjin Im <bj.im@samsung.com>
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
 * @brief   Unit tests of the Server class
 */

#include "ut.hpp"

#include "server.hpp"
#include "exception.hpp"
#include "config/exception.hpp"

#include <string>
#include <thread>

BOOST_AUTO_TEST_SUITE(ServerSuite)

using namespace security_containers;
using namespace security_containers::config;

const std::string TEST_CONFIG_PATH = "/etc/security-containers/tests/server/ut-server/test-daemon.conf";
const std::string BUGGY_CONFIG_PATH = "/etc/security-containers/tests/server/ut-server/buggy-daemon.conf";
const std::string MISSING_CONFIG_PATH = "/this/is/a/missing/file/path/missing-daemon.conf";


BOOST_AUTO_TEST_CASE(ConstructorDestructorTest)
{
    std::unique_ptr<Server> s;
    BOOST_REQUIRE_NO_THROW(s.reset(new Server(TEST_CONFIG_PATH)));
    BOOST_REQUIRE_NO_THROW(s.reset());
}

BOOST_AUTO_TEST_CASE(BuggyConfigTest)
{
    BOOST_REQUIRE_THROW(Server(BUGGY_CONFIG_PATH).run(), ConfigException);
}

BOOST_AUTO_TEST_CASE(MissingConfigTest)
{
    BOOST_REQUIRE_THROW(Server(MISSING_CONFIG_PATH).run(), ConfigException);
}

BOOST_AUTO_TEST_CASE(TerminateTest)
{
    Server s(TEST_CONFIG_PATH);
    BOOST_REQUIRE_NO_THROW(s.terminate());
}

BOOST_AUTO_TEST_CASE(RunTerminateTest)
{
    Server s(TEST_CONFIG_PATH);
    std::thread t([&]() {
        BOOST_REQUIRE_NO_THROW(s.run());
    });
    BOOST_REQUIRE_NO_THROW(s.terminate());
    t.join();
}


BOOST_AUTO_TEST_SUITE_END()
