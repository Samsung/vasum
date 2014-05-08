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
 * @brief   Unit tests of the ContainerAdmin class
 */

#include "ut.hpp"

#include "container-admin.hpp"
#include "exception.hpp"

#include "utils/exception.hpp"
#include "libvirt/exception.hpp"

#include <memory>
#include <string>
#include <thread>
#include <chrono>


BOOST_AUTO_TEST_SUITE(ContainerAdminSuite)

using namespace security_containers;
using namespace security_containers::config;

namespace {

const std::string TEST_CONFIG_PATH = SC_TEST_CONFIG_INSTALL_DIR "/server/ut-container-admin/containers/test.conf";
const std::string BUGGY_CONFIG_PATH = SC_TEST_CONFIG_INSTALL_DIR "/server/ut-container-admin/containers/buggy.conf";
const std::string MISSING_CONFIG_PATH = SC_TEST_CONFIG_INSTALL_DIR "/server/ut-container-admin/containers/missing.conf";

void ensureStarted()
{
    // TODO: fix the libvirt usage so this is not required
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
}

} // namespace


BOOST_AUTO_TEST_CASE(ConstructorTest)
{
    ContainerConfig config; config.parseFile(TEST_CONFIG_PATH);
    BOOST_REQUIRE_NO_THROW(ContainerAdmin ca(config));
}

BOOST_AUTO_TEST_CASE(DestructorTest)
{
    ContainerConfig config; config.parseFile(TEST_CONFIG_PATH);
    std::unique_ptr<ContainerAdmin> ca(new ContainerAdmin(config));
    BOOST_REQUIRE_NO_THROW(ca.reset());
}

BOOST_AUTO_TEST_CASE(BuggyConfigTest)
{
    ContainerConfig config; config.parseFile(BUGGY_CONFIG_PATH);
    BOOST_REQUIRE_THROW(ContainerAdmin ca(config), LibvirtOperationException);
}

BOOST_AUTO_TEST_CASE(MissingConfigTest)
{
    ContainerConfig config; config.parseFile(MISSING_CONFIG_PATH);
    BOOST_REQUIRE_THROW(ContainerAdmin ca(config), UtilsException);
}

BOOST_AUTO_TEST_CASE(StartTest)
{
    ContainerConfig config; config.parseFile(TEST_CONFIG_PATH);
    ContainerAdmin ca(config);
    BOOST_REQUIRE_NO_THROW(ca.start());
    ensureStarted();
    BOOST_CHECK(ca.isRunning());
}

BOOST_AUTO_TEST_CASE(StopTest)
{
    ContainerConfig config; config.parseFile(TEST_CONFIG_PATH);
    ContainerAdmin ca(config);
    BOOST_REQUIRE_NO_THROW(ca.start());
    ensureStarted();
    BOOST_CHECK(ca.isRunning());
    BOOST_REQUIRE_NO_THROW(ca.stop())
    BOOST_CHECK(!ca.isRunning());
    BOOST_CHECK(ca.isStopped());
}

BOOST_AUTO_TEST_CASE(ShutdownTest)
{
    ContainerConfig config; config.parseFile(TEST_CONFIG_PATH);
    ContainerAdmin ca(config);
    BOOST_REQUIRE_NO_THROW(ca.start())
    ensureStarted();
    BOOST_CHECK(ca.isRunning());
    BOOST_REQUIRE_NO_THROW(ca.shutdown())
    // TODO: For this simple configuration, the shutdown signal is ignored
    // BOOST_CHECK(!ca.isRunning());
    // BOOST_CHECK(ca.isStopped());
}

BOOST_AUTO_TEST_CASE(SuspendTest)
{
    ContainerConfig config; config.parseFile(TEST_CONFIG_PATH);
    ContainerAdmin ca(config);
    BOOST_REQUIRE_NO_THROW(ca.start())
    ensureStarted();
    BOOST_CHECK(ca.isRunning());
    BOOST_REQUIRE_NO_THROW(ca.suspend());
    BOOST_CHECK(!ca.isRunning());
    BOOST_CHECK(ca.isPaused());
}

BOOST_AUTO_TEST_CASE(ResumeTest)
{
    ContainerConfig config; config.parseFile(TEST_CONFIG_PATH);
    ContainerAdmin ca(config);
    BOOST_REQUIRE_NO_THROW(ca.start());
    ensureStarted();
    BOOST_REQUIRE_NO_THROW(ca.suspend())
    BOOST_CHECK(ca.isPaused());
    BOOST_REQUIRE_NO_THROW(ca.resume());
    BOOST_CHECK(!ca.isPaused());
    BOOST_CHECK(ca.isRunning());
}

BOOST_AUTO_TEST_CASE(SchedulerLevelTest)
{
    ContainerConfig config; config.parseFile(TEST_CONFIG_PATH);
    ContainerAdmin ca(config);
    BOOST_REQUIRE_NO_THROW(ca.start());
    ensureStarted();
    BOOST_REQUIRE_NO_THROW(ca.setSchedulerLevel(SchedulerLevel::FOREGROUND));
    BOOST_REQUIRE(ca.getSchedulerQuota() == config.cpuQuotaForeground);
    BOOST_REQUIRE_NO_THROW(ca.setSchedulerLevel(SchedulerLevel::BACKGROUND));
    BOOST_REQUIRE(ca.getSchedulerQuota() == config.cpuQuotaBackground);
}

BOOST_AUTO_TEST_SUITE_END()
