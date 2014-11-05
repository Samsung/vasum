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

#include "config.hpp"
#include "ut.hpp"

#include "container-admin.hpp"
#include "exception.hpp"

#include "utils/glib-loop.hpp"
#include "utils/scoped-dir.hpp"
#include "config/manager.hpp"

using namespace security_containers;

namespace {

const std::string TEST_CONFIG_PATH = SC_TEST_CONFIG_INSTALL_DIR "/server/ut-container-admin/containers/test.conf";
const std::string TEST_NO_SHUTDOWN_CONFIG_PATH = SC_TEST_CONFIG_INSTALL_DIR "/server/ut-container-admin/containers/test-no-shutdown.conf";
const std::string BUGGY_CONFIG_PATH = SC_TEST_CONFIG_INSTALL_DIR "/server/ut-container-admin/containers/buggy.conf";
const std::string MISSING_CONFIG_PATH = SC_TEST_CONFIG_INSTALL_DIR "/server/ut-container-admin/containers/missing.conf";
const std::string CONTAINERS_PATH = "/tmp/ut-containers";
const std::string LXC_TEMPLATES_PATH = SC_TEST_LXC_TEMPLATES_INSTALL_DIR;

struct Fixture {
    utils::ScopedGlibLoop mLoop;
    utils::ScopedDir mContainersPathGuard = CONTAINERS_PATH;

    ContainerConfig mConfig;

    std::unique_ptr<ContainerAdmin> create(const std::string& configPath)
    {
        config::loadFromFile(configPath, mConfig);
        return std::unique_ptr<ContainerAdmin>(new ContainerAdmin(CONTAINERS_PATH,
                                                                  LXC_TEMPLATES_PATH,
                                                                  mConfig));
    }

    void ensureStarted()
    {
        // wait for containers init to fully start
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
};

} // namespace


BOOST_FIXTURE_TEST_SUITE(ContainerAdminSuite, Fixture)

BOOST_AUTO_TEST_CASE(ConstructorDestructorTest)
{
    auto admin = create(TEST_CONFIG_PATH);
    admin.reset();
}

BOOST_AUTO_TEST_CASE(MissingConfigTest)
{
    BOOST_REQUIRE_THROW(create(MISSING_CONFIG_PATH), ContainerOperationException);
}

BOOST_AUTO_TEST_CASE(StartTest)
{
    auto admin = create(TEST_CONFIG_PATH);

    admin->start();
    ensureStarted();

    BOOST_CHECK(admin->isRunning());
}

BOOST_AUTO_TEST_CASE(StartBuggyTest)
{
    auto admin = create(BUGGY_CONFIG_PATH);
    BOOST_REQUIRE_THROW(admin->start(), ContainerOperationException);
}

BOOST_AUTO_TEST_CASE(StopShutdownTest)
{
    auto admin = create(TEST_CONFIG_PATH);

    admin->start();
    ensureStarted();
    BOOST_REQUIRE(admin->isRunning());

    admin->stop();
    BOOST_CHECK(!admin->isRunning());
    BOOST_CHECK(admin->isStopped());
}

// This test needs to wait for a shutdown timer in stop() method. This takes 10s+.
BOOST_AUTO_TEST_CASE(StopDestroyTest)
{
    auto admin = create(TEST_NO_SHUTDOWN_CONFIG_PATH);

    admin->start();
    ensureStarted();
    BOOST_REQUIRE(admin->isRunning());

    admin->stop();
    BOOST_CHECK(!admin->isRunning());
    BOOST_CHECK(admin->isStopped());
}

BOOST_AUTO_TEST_CASE(SuspendResumeTest)
{
    auto admin = create(TEST_NO_SHUTDOWN_CONFIG_PATH);

    admin->start();
    ensureStarted();
    BOOST_REQUIRE(admin->isRunning());

    admin->suspend();
    BOOST_CHECK(!admin->isRunning());
    BOOST_CHECK(!admin->isStopped());
    BOOST_CHECK(admin->isPaused());

    admin->resume();
    BOOST_CHECK(!admin->isPaused());
    BOOST_CHECK(!admin->isStopped());
    BOOST_CHECK(admin->isRunning());
}

//BOOST_AUTO_TEST_CASE(SchedulerLevelTest)
//{
//    auto admin = create(TEST_CONFIG_PATH);
//
//    admin->start();
//    ensureStarted();
//    BOOST_REQUIRE_NO_THROW(admin->setSchedulerLevel(SchedulerLevel::FOREGROUND));
//    BOOST_REQUIRE(admin->getSchedulerQuota() == config.cpuQuotaForeground);
//    BOOST_REQUIRE_NO_THROW(admin->setSchedulerLevel(SchedulerLevel::BACKGROUND));
//    BOOST_REQUIRE(admin->getSchedulerQuota() == config.cpuQuotaBackground);
//}

BOOST_AUTO_TEST_SUITE_END()
