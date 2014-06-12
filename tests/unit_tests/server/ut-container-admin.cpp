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

#include "utils/latch.hpp"
#include "utils/glib-loop.hpp"
#include "utils/exception.hpp"
#include "utils/callback-guard.hpp"
#include "libvirt/exception.hpp"
#include "config/manager.hpp"

#include <memory>
#include <string>
#include <thread>
#include <chrono>


using namespace security_containers;

namespace {

const std::string TEST_CONFIG_PATH = SC_TEST_CONFIG_INSTALL_DIR "/server/ut-container-admin/containers/test.conf";
const std::string TEST_NO_SHUTDOWN_CONFIG_PATH = SC_TEST_CONFIG_INSTALL_DIR "/server/ut-container-admin/containers/test-no-shutdown.conf";
const std::string BUGGY_CONFIG_PATH = SC_TEST_CONFIG_INSTALL_DIR "/server/ut-container-admin/containers/buggy.conf";
const std::string MISSING_CONFIG_PATH = SC_TEST_CONFIG_INSTALL_DIR "/server/ut-container-admin/containers/missing.conf";
const unsigned int WAIT_TIMEOUT = 5 * 1000;
const unsigned int WAIT_STOP_TIMEOUT = 15 * 1000;

void ensureStarted()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
}

struct Fixture {
    utils::ScopedGlibLoop mLoop;
    utils::CallbackGuard mGuard;
};

} // namespace


BOOST_FIXTURE_TEST_SUITE(ContainerAdminSuite, Fixture)

BOOST_AUTO_TEST_CASE(ConstructorDestructorTest)
{
    ContainerConfig config;
    config::loadFromFile(TEST_CONFIG_PATH, config);
    std::unique_ptr<ContainerAdmin> admin;
    BOOST_REQUIRE_NO_THROW(admin.reset(new ContainerAdmin(config)));
    BOOST_REQUIRE_NO_THROW(admin.reset());
}

BOOST_AUTO_TEST_CASE(BuggyConfigTest)
{
    ContainerConfig config;
    config::loadFromFile(BUGGY_CONFIG_PATH, config);
    BOOST_REQUIRE_THROW(ContainerAdmin ca(config), LibvirtOperationException);
}

BOOST_AUTO_TEST_CASE(MissingConfigTest)
{
    ContainerConfig config;
    config::loadFromFile(MISSING_CONFIG_PATH, config);
    BOOST_REQUIRE_THROW(ContainerAdmin ca(config), UtilsException);
}

BOOST_AUTO_TEST_CASE(StartTest)
{
    utils::Latch booted;
    ContainerAdmin::ListenerId id = ContainerAdmin::LISTENER_ID_INVALID;
    ContainerConfig config;
    config::loadFromFile(TEST_CONFIG_PATH, config);
    ContainerAdmin ca(config);

    ContainerAdmin::LifecycleListener bootedListener = [&](const int event, const int detail) {
        if (event == VIR_DOMAIN_EVENT_STARTED && detail == VIR_DOMAIN_EVENT_STARTED_BOOTED) {
            booted.set();
        }
    };
    BOOST_REQUIRE_NO_THROW(id = ca.registerLifecycleListener(bootedListener, mGuard.spawn()));

    BOOST_REQUIRE_NO_THROW(ca.start());
    ensureStarted();

    BOOST_CHECK(booted.wait(WAIT_TIMEOUT));
    BOOST_CHECK(ca.isRunning());

    BOOST_REQUIRE_NO_THROW(ca.removeListener(id));
}

BOOST_AUTO_TEST_CASE(ShutdownTest)
{
    utils::Latch shutdown;
    ContainerAdmin::ListenerId id = ContainerAdmin::LISTENER_ID_INVALID;
    ContainerConfig config;
    config::loadFromFile(TEST_CONFIG_PATH, config);
    ContainerAdmin ca(config);

    ContainerAdmin::LifecycleListener shutdownListener = [&](const int event, const int detail) {
        if (event == VIR_DOMAIN_EVENT_STOPPED && detail == VIR_DOMAIN_EVENT_STOPPED_SHUTDOWN) {
            shutdown.set();
        }
    };

    BOOST_REQUIRE_NO_THROW(ca.start());
    ensureStarted();
    BOOST_REQUIRE(ca.isRunning());
    BOOST_REQUIRE_NO_THROW(id = ca.registerLifecycleListener(shutdownListener, mGuard.spawn()));

    BOOST_REQUIRE_NO_THROW(ca.shutdown());
    BOOST_CHECK(shutdown.wait(WAIT_TIMEOUT));
    BOOST_CHECK(!ca.isRunning());
    BOOST_CHECK(ca.isStopped());

    BOOST_REQUIRE_NO_THROW(ca.removeListener(id));
}

BOOST_AUTO_TEST_CASE(DestroyTest)
{
    utils::Latch destroyed;
    ContainerAdmin::ListenerId id = ContainerAdmin::LISTENER_ID_INVALID;
    ContainerConfig config;
    config::loadFromFile(TEST_CONFIG_PATH, config);
    ContainerAdmin ca(config);

    ContainerAdmin::LifecycleListener destroyedListener = [&](const int event, const int detail) {
        if (event == VIR_DOMAIN_EVENT_STOPPED && detail == VIR_DOMAIN_EVENT_STOPPED_DESTROYED) {
            destroyed.set();
        }
    };

    BOOST_REQUIRE_NO_THROW(ca.start());
    ensureStarted();
    BOOST_REQUIRE(ca.isRunning());
    BOOST_REQUIRE_NO_THROW(id = ca.registerLifecycleListener(destroyedListener, mGuard.spawn()));

    BOOST_REQUIRE_NO_THROW(ca.destroy());
    BOOST_CHECK(destroyed.wait(WAIT_TIMEOUT));
    BOOST_CHECK(!ca.isRunning());
    BOOST_CHECK(ca.isStopped());

    BOOST_REQUIRE_NO_THROW(ca.removeListener(id));
}

BOOST_AUTO_TEST_CASE(StopShutdownTest)
{
    utils::Latch shutdown;
    ContainerAdmin::ListenerId id = ContainerAdmin::LISTENER_ID_INVALID;
    ContainerConfig config;
    config::loadFromFile(TEST_CONFIG_PATH, config);
    ContainerAdmin ca(config);

    ContainerAdmin::LifecycleListener shutdownListener = [&](const int event, const int detail) {
        if (event == VIR_DOMAIN_EVENT_STOPPED && detail == VIR_DOMAIN_EVENT_STOPPED_SHUTDOWN) {
            shutdown.set();
        }
    };

    BOOST_REQUIRE_NO_THROW(ca.start());
    ensureStarted();
    BOOST_REQUIRE(ca.isRunning());
    BOOST_REQUIRE_NO_THROW(id = ca.registerLifecycleListener(shutdownListener, mGuard.spawn()));

    BOOST_REQUIRE_NO_THROW(ca.stop());
    BOOST_CHECK(shutdown.wait(WAIT_TIMEOUT));
    BOOST_CHECK(!ca.isRunning());
    BOOST_CHECK(ca.isStopped());

    BOOST_REQUIRE_NO_THROW(ca.removeListener(id));
}

// This test needs to wait for a shutdown timer in stop() method. This takes 10s+.
BOOST_AUTO_TEST_CASE(StopDestroyTest)
{
    utils::Latch destroyed;
    ContainerAdmin::ListenerId id = ContainerAdmin::LISTENER_ID_INVALID;
    ContainerConfig config;
    config::loadFromFile(TEST_NO_SHUTDOWN_CONFIG_PATH, config);
    ContainerAdmin ca(config);

    ContainerAdmin::LifecycleListener destroyedListener = [&](const int event, const int detail) {
        if (event == VIR_DOMAIN_EVENT_STOPPED && detail == VIR_DOMAIN_EVENT_STOPPED_DESTROYED) {
            destroyed.set();
        }
    };

    BOOST_REQUIRE_NO_THROW(ca.start());
    ensureStarted();
    BOOST_REQUIRE(ca.isRunning());
    BOOST_REQUIRE_NO_THROW(id = ca.registerLifecycleListener(destroyedListener, mGuard.spawn()));

    BOOST_REQUIRE_NO_THROW(ca.stop());
    BOOST_CHECK(destroyed.wait(WAIT_STOP_TIMEOUT));
    BOOST_CHECK(!ca.isRunning());
    BOOST_CHECK(ca.isStopped());

    BOOST_REQUIRE_NO_THROW(ca.removeListener(id));
}

BOOST_AUTO_TEST_CASE(SuspendTest)
{
    utils::Latch paused;
    ContainerAdmin::ListenerId id = ContainerAdmin::LISTENER_ID_INVALID;
    ContainerConfig config;
    config::loadFromFile(TEST_CONFIG_PATH, config);
    ContainerAdmin ca(config);

    ContainerAdmin::LifecycleListener pausedListener = [&](const int event, const int detail) {
        if (event == VIR_DOMAIN_EVENT_SUSPENDED && detail == VIR_DOMAIN_EVENT_SUSPENDED_PAUSED) {
            paused.set();
        }
    };

    BOOST_REQUIRE_NO_THROW(ca.start())
    ensureStarted();
    BOOST_REQUIRE(ca.isRunning());
    BOOST_REQUIRE_NO_THROW(id = ca.registerLifecycleListener(pausedListener, mGuard.spawn()));

    BOOST_REQUIRE_NO_THROW(ca.suspend());
    BOOST_CHECK(paused.wait(WAIT_TIMEOUT));
    BOOST_CHECK(!ca.isRunning());
    BOOST_CHECK(ca.isPaused());

    BOOST_REQUIRE_NO_THROW(ca.removeListener(id));
}

BOOST_AUTO_TEST_CASE(ResumeTest)
{
    utils::Latch unpaused;
    ContainerAdmin::ListenerId id = ContainerAdmin::LISTENER_ID_INVALID;
    ContainerConfig config;
    config::loadFromFile(TEST_CONFIG_PATH, config);
    ContainerAdmin ca(config);

    ContainerAdmin::LifecycleListener unpausedListener = [&](const int event, const int detail) {
        if (event == VIR_DOMAIN_EVENT_RESUMED && detail == VIR_DOMAIN_EVENT_RESUMED_UNPAUSED) {
            unpaused.set();
        }
    };

    BOOST_REQUIRE_NO_THROW(ca.start());
    ensureStarted();
    BOOST_REQUIRE(ca.isRunning());
    BOOST_REQUIRE_NO_THROW(ca.suspend())
    BOOST_REQUIRE(ca.isPaused());
    BOOST_REQUIRE_NO_THROW(id = ca.registerLifecycleListener(unpausedListener, mGuard.spawn()));

    BOOST_REQUIRE_NO_THROW(ca.resume());
    BOOST_CHECK(unpaused.wait(WAIT_TIMEOUT));
    BOOST_CHECK(!ca.isPaused());
    BOOST_CHECK(ca.isRunning());

    BOOST_REQUIRE_NO_THROW(ca.removeListener(id));
}

BOOST_AUTO_TEST_CASE(SchedulerLevelTest)
{
    ContainerConfig config;
    config::loadFromFile(TEST_CONFIG_PATH, config);
    ContainerAdmin ca(config);
    BOOST_REQUIRE_NO_THROW(ca.start());
    ensureStarted();
    BOOST_REQUIRE_NO_THROW(ca.setSchedulerLevel(SchedulerLevel::FOREGROUND));
    BOOST_REQUIRE(ca.getSchedulerQuota() == config.cpuQuotaForeground);
    BOOST_REQUIRE_NO_THROW(ca.setSchedulerLevel(SchedulerLevel::BACKGROUND));
    BOOST_REQUIRE(ca.getSchedulerQuota() == config.cpuQuotaBackground);
}

BOOST_AUTO_TEST_SUITE_END()
