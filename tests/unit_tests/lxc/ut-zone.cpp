/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Piotr Bartosiewicz <p.bartosiewi@partner.samsung.com>
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
 * @author  Piotr Bartosiewicz (p.bartosiewi@partner.samsung.com)
 * @brief   Unit tests of LxcZone class
 */

#include "config.hpp"
#include "ut.hpp"

#include "lxc/zone.hpp"
#include "lxc/exception.hpp"
#include "utils/scoped-dir.hpp"

#include <fcntl.h>
#include <thread>
#include <chrono>
#include <boost/filesystem.hpp>

namespace {

using namespace vasum;
using namespace vasum::lxc;

const std::string ZONE_PATH = "/tmp/ut-zone/";
const std::string ZONE_NAME = "ut-zone";
const std::string ZONE_TEMPLATE = VSM_TEST_TEMPLATES_INSTALL_DIR "/minimal.sh";
const char* TEMPLATE_ARGS[] = {NULL};
const std::int32_t DEFAULT_FILE_MODE = 0666;

struct Fixture {
    utils::ScopedDir mLxcDirGuard;

    Fixture()
        : mLxcDirGuard(ZONE_PATH)
    {
        cleanup();
    }

    ~Fixture()
    {
        cleanup();
    }

    static void cleanup()
    {
        LxcZone lxc(ZONE_PATH, ZONE_NAME);
        if (lxc.isDefined()) {
            if (lxc.getState() != LxcZone::State::STOPPED) {
                lxc.stop();
            }
            lxc.destroy();
        }
    }

    static void waitForInit()
    {
        // wait for init fully started (wait for bash to be able to trap SIGTERM)
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
};

} // namespace

BOOST_FIXTURE_TEST_SUITE(LxcZoneSuite, Fixture)

BOOST_AUTO_TEST_CASE(ConstructorDestructor)
{
    LxcZone lxc(ZONE_PATH, ZONE_NAME);
}

BOOST_AUTO_TEST_CASE(CreateDestroy)
{
    LxcZone lxc(ZONE_PATH, ZONE_NAME);
    BOOST_CHECK(!lxc.isDefined());

    BOOST_CHECK(lxc.create(ZONE_TEMPLATE, TEMPLATE_ARGS));

    BOOST_CHECK(lxc.isDefined());
    BOOST_CHECK_EQUAL(lxc.getConfigItem("lxc.rootfs"), ZONE_PATH + ZONE_NAME + "/rootfs");
    BOOST_CHECK_THROW(lxc.getConfigItem("xxx"), KeyNotFoundException);

    BOOST_CHECK(lxc.destroy());

    BOOST_CHECK(!lxc.isDefined());
}

BOOST_AUTO_TEST_CASE(StartShutdown)
{
    {
        LxcZone lxc(ZONE_PATH, ZONE_NAME);
        BOOST_CHECK(lxc.create(ZONE_TEMPLATE, TEMPLATE_ARGS));
    }
    LxcZone lxc(ZONE_PATH, ZONE_NAME);
    BOOST_CHECK(lxc.getState() == LxcZone::State::STOPPED);
    const char* argv[] = {
        "/bin/bash",
        "-c",
        "trap exit SIGTERM; while true; do sleep 0.1; done",
        NULL
    };
    BOOST_CHECK(lxc.start(argv));
    BOOST_CHECK(lxc.getState() == LxcZone::State::RUNNING);
    waitForInit();
    BOOST_CHECK(lxc.shutdown(2));
    BOOST_CHECK(lxc.getState() == LxcZone::State::STOPPED);

    BOOST_CHECK(lxc.destroy());
}

BOOST_AUTO_TEST_CASE(StartStop)
{
    {
        LxcZone lxc(ZONE_PATH, ZONE_NAME);
        BOOST_CHECK(lxc.create(ZONE_TEMPLATE, TEMPLATE_ARGS));
    }
    LxcZone lxc(ZONE_PATH, ZONE_NAME);
    BOOST_CHECK(lxc.getState() == LxcZone::State::STOPPED);
    const char* argv[] = {
        "/bin/bash",
        "-c",
        "trap exit SIGTERM; while true; do sleep 0.1; done",
        NULL
    };
    BOOST_CHECK(lxc.start(argv));
    BOOST_CHECK(lxc.getState() == LxcZone::State::RUNNING);
    waitForInit();
#ifndef USE_EXEC // TODO improve shutdown implementation
    BOOST_CHECK(!lxc.shutdown(1));
    BOOST_CHECK(lxc.getState() == LxcZone::State::RUNNING);
#endif
    BOOST_CHECK(lxc.stop());
    BOOST_CHECK(lxc.getState() == LxcZone::State::STOPPED);

    BOOST_CHECK(lxc.destroy());
}

BOOST_AUTO_TEST_CASE(StartHasStopped)
{
    {
        LxcZone lxc(ZONE_PATH, ZONE_NAME);
        BOOST_CHECK(lxc.create(ZONE_TEMPLATE, TEMPLATE_ARGS));
    }
    LxcZone lxc(ZONE_PATH, ZONE_NAME);
    BOOST_CHECK(lxc.getState() == LxcZone::State::STOPPED);
    const char* argv[] = {
        "/bin/bash",
        "-c",
        "sleep 0.4",
        NULL
    };
    BOOST_CHECK(lxc.start(argv));

    waitForInit();
    BOOST_CHECK(lxc.getState() == LxcZone::State::RUNNING);

    // wait for the zone process to exit (200ms of time reserve)
    std::this_thread::sleep_for(std::chrono::milliseconds(400));
    BOOST_CHECK(lxc.getState() == LxcZone::State::STOPPED);

    BOOST_CHECK(lxc.destroy());
}

BOOST_AUTO_TEST_CASE(FreezeUnfreeze)
{
    LxcZone lxc(ZONE_PATH, ZONE_NAME);
    BOOST_CHECK(lxc.create(ZONE_TEMPLATE, TEMPLATE_ARGS));
    const char* argv[] = {
        "/bin/bash",
        "-c",
        "trap exit SIGTERM; while true; do sleep 0.1; done",
        NULL
    };
    BOOST_CHECK(lxc.start(argv));
    BOOST_CHECK(lxc.getState() == LxcZone::State::RUNNING);
    waitForInit();
    BOOST_CHECK(lxc.freeze());
    BOOST_CHECK(lxc.getState() == LxcZone::State::FROZEN);
    BOOST_CHECK(lxc.unfreeze());
    BOOST_CHECK(lxc.getState() == LxcZone::State::RUNNING);
    BOOST_CHECK(lxc.shutdown(2));
    BOOST_CHECK(lxc.getState() == LxcZone::State::STOPPED);

    BOOST_CHECK(lxc.destroy());
}

BOOST_AUTO_TEST_CASE(FreezeStop)
{
    LxcZone lxc(ZONE_PATH, ZONE_NAME);
    BOOST_CHECK(lxc.create(ZONE_TEMPLATE, TEMPLATE_ARGS));
    const char* argv[] = {
        "/bin/bash",
        "-c",
        "trap exit SIGTERM; while true; do sleep 0.1; done",
        NULL
    };
    BOOST_CHECK(lxc.start(argv));
    BOOST_CHECK(lxc.getState() == LxcZone::State::RUNNING);
    waitForInit();
    BOOST_CHECK(lxc.freeze());
    BOOST_CHECK(lxc.getState() == LxcZone::State::FROZEN);
    BOOST_CHECK(!lxc.shutdown(1));
    BOOST_CHECK(lxc.getState() == LxcZone::State::FROZEN);
    BOOST_CHECK(lxc.stop());
    BOOST_CHECK(lxc.getState() == LxcZone::State::STOPPED);

    BOOST_CHECK(lxc.destroy());
}

BOOST_AUTO_TEST_CASE(Repeat)
{
    LxcZone lxc(ZONE_PATH, ZONE_NAME);
    BOOST_CHECK(lxc.create(ZONE_TEMPLATE, TEMPLATE_ARGS));
    BOOST_CHECK(!lxc.create(ZONE_TEMPLATE, TEMPLATE_ARGS));// forbidden
    const char* argv[] = {
        "/bin/bash",
        "-c",
        "trap exit SIGTERM; while true; do sleep 0.1; done",
        NULL
    };
    BOOST_CHECK(lxc.start(argv));
    BOOST_CHECK(lxc.getState() == LxcZone::State::RUNNING);
    waitForInit();
    BOOST_CHECK(!lxc.start(argv)); // forbidden
    BOOST_CHECK(lxc.freeze());
    BOOST_CHECK(lxc.getState() == LxcZone::State::FROZEN);
    BOOST_CHECK(lxc.freeze()); // repeat is nop
    BOOST_CHECK(lxc.getState() == LxcZone::State::FROZEN);
    BOOST_CHECK(lxc.unfreeze());
    BOOST_CHECK(lxc.getState() == LxcZone::State::RUNNING);
    BOOST_CHECK(lxc.unfreeze()); // repeat is nop
    BOOST_CHECK(lxc.getState() == LxcZone::State::RUNNING);
    BOOST_CHECK(lxc.stop());
    BOOST_CHECK(lxc.getState() == LxcZone::State::STOPPED);
    BOOST_CHECK(lxc.stop()); // repeat is nop
    BOOST_CHECK(lxc.getState() == LxcZone::State::STOPPED);

    BOOST_CHECK(lxc.destroy());
    BOOST_CHECK(!lxc.isDefined());
    BOOST_CHECK(!lxc.destroy()); // forbidden (why?)
}

BOOST_AUTO_TEST_CASE(CreateFile)
{
    // Create and start the container:
    LxcZone lxc(ZONE_PATH, ZONE_NAME);
    BOOST_REQUIRE(lxc.create(ZONE_TEMPLATE, TEMPLATE_ARGS));
    const char* argv[] = {
        "/bin/bash",
        "-c",
        "trap exit SIGTERM; while true; do sleep 0.1; done",
        NULL
    };
    BOOST_REQUIRE(lxc.start(argv));
    BOOST_REQUIRE(lxc.getState() == LxcZone::State::RUNNING);
    waitForInit();

    // The test
    int fd;
    BOOST_REQUIRE(lxc.createFile("./112.txt", O_RDWR, DEFAULT_FILE_MODE, &fd));
    BOOST_REQUIRE(::fcntl(fd, F_GETFD) != -1);
    BOOST_REQUIRE(::close(fd) != -1);

    BOOST_REQUIRE(lxc.createFile("/2.txt", O_RDONLY, DEFAULT_FILE_MODE, &fd));
    BOOST_REQUIRE(::fcntl(fd, F_GETFD) != -1);
    BOOST_REQUIRE(::close(fd) != -1);

    BOOST_REQUIRE(lxc.createFile("/3.txt", O_WRONLY, DEFAULT_FILE_MODE, &fd));
    BOOST_REQUIRE(::fcntl(fd, F_GETFD) != -1);
    BOOST_REQUIRE(::close(fd) != -1);

    // Close
    BOOST_REQUIRE(lxc.stop());
    BOOST_REQUIRE(lxc.getState() == LxcZone::State::STOPPED);
    BOOST_REQUIRE(lxc.destroy());
}

BOOST_AUTO_TEST_SUITE_END()
