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
 * @brief   Unit tests of the ZoneAdmin class
 */

#include "config.hpp"
#include "ut.hpp"

#include "zone-admin.hpp"
#include "exception.hpp"

#include "utils/glib-loop.hpp"
#include "utils/scoped-dir.hpp"
#include "config/manager.hpp"

using namespace vasum;

namespace {

const std::string TEMPLATES_DIR = VSM_TEST_CONFIG_INSTALL_DIR "/server/ut-zone-admin/templates";
const std::string TEST_CONFIG_PATH = TEMPLATES_DIR + "/test.conf";
const std::string TEST_NO_SHUTDOWN_CONFIG_PATH = TEMPLATES_DIR + "/test-no-shutdown.conf";
const std::string BUGGY_CONFIG_PATH = TEMPLATES_DIR + "/buggy.conf";
const std::string MISSING_CONFIG_PATH = TEMPLATES_DIR + "/missing.conf";
const std::string ZONES_PATH = "/tmp/ut-zones";
const std::string LXC_TEMPLATES_PATH = VSM_TEST_LXC_TEMPLATES_INSTALL_DIR;

struct Fixture {
    utils::ScopedGlibLoop mLoop;
    utils::ScopedDir mZonesPathGuard;

    ZoneConfig mConfig;
    ZoneDynamicConfig mDynamicConfig;

    Fixture()
        : mZonesPathGuard(ZONES_PATH)
    {}

    std::unique_ptr<ZoneAdmin> create(const std::string& configPath)
    {
        config::loadFromJsonFile(configPath, mConfig);
        config::loadFromJsonFile(configPath, mDynamicConfig);
        return std::unique_ptr<ZoneAdmin>(new ZoneAdmin("zoneId",
                                                        ZONES_PATH,
                                                        LXC_TEMPLATES_PATH,
                                                        mConfig,
                                                        mDynamicConfig));
    }

    void ensureStarted()
    {
        // wait for zones init to fully start
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
};

} // namespace


BOOST_FIXTURE_TEST_SUITE(ZoneAdminSuite, Fixture)

BOOST_AUTO_TEST_CASE(ConstructorDestructorTest)
{
    auto admin = create(TEST_CONFIG_PATH);
    admin.reset();
}

BOOST_AUTO_TEST_CASE(MissingConfigTest)
{
    BOOST_REQUIRE_EXCEPTION(create(MISSING_CONFIG_PATH),
                            ZoneOperationException,
                            WhatEquals("Could not create zone"));
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
    BOOST_REQUIRE_EXCEPTION(admin->start(),
                            ZoneOperationException,
                            WhatEquals("Could not start zone"));
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

BOOST_AUTO_TEST_CASE(SchedulerLevelTest)
{
    BOOST_REQUIRE(mConfig.cpuQuotaForeground != mConfig.cpuQuotaBackground);

    auto admin = create(TEST_CONFIG_PATH);

    admin->start();
    ensureStarted();

    admin->setSchedulerLevel(SchedulerLevel::FOREGROUND);
    BOOST_CHECK_EQUAL(admin->getSchedulerQuota(), mConfig.cpuQuotaForeground);

    admin->setSchedulerLevel(SchedulerLevel::BACKGROUND);
    BOOST_CHECK_EQUAL(admin->getSchedulerQuota(), mConfig.cpuQuotaBackground);
}

BOOST_AUTO_TEST_SUITE_END()
