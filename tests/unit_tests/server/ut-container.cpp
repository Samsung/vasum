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
 * @brief   Unit tests of the Container class
 */

#include "ut.hpp"

#include "container.hpp"
#include "exception.hpp"

#include "utils/exception.hpp"
#include "utils/glib-loop.hpp"

#include <memory>
#include <string>


using namespace security_containers;
using namespace security_containers::config;

namespace {

const std::string TEST_CONFIG_PATH = SC_TEST_CONFIG_INSTALL_DIR "/server/ut-container/containers/test.conf";
const std::string TEST_DBUS_CONFIG_PATH = SC_TEST_CONFIG_INSTALL_DIR "/server/ut-container/containers/test-dbus.conf";
const std::string BUGGY_CONFIG_PATH = SC_TEST_CONFIG_INSTALL_DIR "/server/ut-container/containers/buggy.conf";
const std::string MISSING_CONFIG_PATH = "/this/is/a/missing/file/path/config.conf";

struct Fixture {
    utils::ScopedGlibLoop mLoop;
};

} // namespace


BOOST_FIXTURE_TEST_SUITE(ContainerSuite, Fixture)

BOOST_AUTO_TEST_CASE(ConstructorTest)
{
    BOOST_REQUIRE_NO_THROW(Container c(TEST_CONFIG_PATH));
}

BOOST_AUTO_TEST_CASE(DestructorTest)
{
    std::unique_ptr<Container> c(new Container(TEST_CONFIG_PATH));
    BOOST_REQUIRE_NO_THROW(c.reset());
}

BOOST_AUTO_TEST_CASE(BuggyConfigTest)
{
    BOOST_REQUIRE_THROW(Container c(BUGGY_CONFIG_PATH), UtilsException);
}

BOOST_AUTO_TEST_CASE(MissingConfigTest)
{
    BOOST_REQUIRE_THROW(Container c(MISSING_CONFIG_PATH), ConfigException);
}

BOOST_AUTO_TEST_CASE(StartStopTest)
{
    std::unique_ptr<Container> c(new Container(TEST_CONFIG_PATH));
    BOOST_REQUIRE_NO_THROW(c->start());
    BOOST_REQUIRE_NO_THROW(c->stop());
}

BOOST_AUTO_TEST_CASE(DbusConnectionTest)
{
    std::unique_ptr<Container> c;
    BOOST_REQUIRE_NO_THROW(c.reset(new Container(TEST_DBUS_CONFIG_PATH)));
    BOOST_REQUIRE_NO_THROW(c->start());
    BOOST_REQUIRE_NO_THROW(c->stop());
}

// TODO: DbusReconnectionTest


BOOST_AUTO_TEST_SUITE_END()
