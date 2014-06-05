/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Jan Olszak <j.olszak@samsung.com>
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
 * @brief   Unit tests of the ContainersManager class
 */

#include "config.hpp"
#include "ut.hpp"

#include "containers-manager.hpp"
#include "exception.hpp"

#include "utils/glib-loop.hpp"
#include "config/exception.hpp"

#include <memory>
#include <string>


using namespace security_containers;
using namespace security_containers::config;

namespace {

const std::string TEST_CONFIG_PATH = SC_TEST_CONFIG_INSTALL_DIR "/server/ut-containers-manager/test-daemon.conf";
const std::string BUGGY_CONFIG_PATH = SC_TEST_CONFIG_INSTALL_DIR "/server/ut-containers-manager/buggy-daemon.conf";
const std::string BUGGY_FOREGROUND_CONFIG_PATH = SC_TEST_CONFIG_INSTALL_DIR "/server/ut-containers-manager/buggy-foreground-daemon.conf";
const std::string MISSING_CONFIG_PATH = "/this/is/a/missing/file/path/missing-daemon.conf";

struct Fixture {
    utils::ScopedGlibLoop mLoop;
};

} // namespace


BOOST_FIXTURE_TEST_SUITE(ContainersManagerSuite, Fixture)

BOOST_AUTO_TEST_CASE(ConstructorTest)
{
    BOOST_REQUIRE_NO_THROW(ContainersManager cm(TEST_CONFIG_PATH););
}

BOOST_AUTO_TEST_CASE(DestructorTest)
{
    std::unique_ptr<ContainersManager> cm(new ContainersManager(TEST_CONFIG_PATH));
    BOOST_REQUIRE_NO_THROW(cm.reset());
}

BOOST_AUTO_TEST_CASE(BuggyConfigTest)
{
    BOOST_REQUIRE_THROW(ContainersManager cm(BUGGY_CONFIG_PATH), ConfigException);
}

BOOST_AUTO_TEST_CASE(MissingConfigTest)
{
    BOOST_REQUIRE_THROW(ContainersManager cm(MISSING_CONFIG_PATH), ConfigException);
}

BOOST_AUTO_TEST_CASE(StartAllTest)
{
    ContainersManager cm(TEST_CONFIG_PATH);
    BOOST_REQUIRE_NO_THROW(cm.startAll());
    BOOST_CHECK(cm.getRunningForegroundContainerId() == "ut-containers-manager-console1");
}

BOOST_AUTO_TEST_CASE(BuggyForegroundTest)
{
    ContainersManager cm(BUGGY_FOREGROUND_CONFIG_PATH);
    BOOST_REQUIRE_NO_THROW(cm.startAll());
    BOOST_CHECK(cm.getRunningForegroundContainerId() == "ut-containers-manager-console2");
}

BOOST_AUTO_TEST_CASE(StopAllTest)
{
    ContainersManager cm(TEST_CONFIG_PATH);
    BOOST_REQUIRE_NO_THROW(cm.startAll());
    BOOST_REQUIRE_NO_THROW(cm.stopAll());
    BOOST_CHECK(cm.getRunningForegroundContainerId().empty());
}

BOOST_AUTO_TEST_CASE(FocusTest)
{
    ContainersManager cm(TEST_CONFIG_PATH);
    BOOST_REQUIRE_NO_THROW(cm.startAll());
    BOOST_REQUIRE_NO_THROW(cm.focus("ut-containers-manager-console2"));
    BOOST_CHECK(cm.getRunningForegroundContainerId() == "ut-containers-manager-console2");
    BOOST_REQUIRE_NO_THROW(cm.focus("ut-containers-manager-console1"));
    BOOST_CHECK(cm.getRunningForegroundContainerId() == "ut-containers-manager-console1");
    BOOST_REQUIRE_NO_THROW(cm.focus("ut-containers-manager-console3"));
    BOOST_CHECK(cm.getRunningForegroundContainerId() == "ut-containers-manager-console3");
}

BOOST_AUTO_TEST_SUITE_END()
