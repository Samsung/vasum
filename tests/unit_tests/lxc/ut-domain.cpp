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
 * @brief   Unit tests of LxcDomain class
 */

#include "config.hpp"
#include "ut.hpp"

#include "lxc/domain.hpp"
#include "lxc/exception.hpp"
#include "utils/scoped-dir.hpp"

#include <thread>
#include <chrono>
#include <boost/filesystem.hpp>

namespace {

using namespace security_containers;
using namespace security_containers::lxc;

const std::string LXC_PATH = "/tmp/ut-lxc/";
const std::string DOMAIN_NAME = "ut-domain";
const std::string TEMPLATE = SC_TEST_LXC_TEMPLATES_INSTALL_DIR "/minimal.sh";
const char* TEMPLATE_ARGS[] = {NULL};

struct Fixture {
    utils::ScopedDir mLxcDirGuard;

    Fixture()
        : mLxcDirGuard(LXC_PATH)
    {
        cleanup();
    }

    ~Fixture()
    {
        cleanup();
    }

    void cleanup()
    {
        LxcDomain lxc(LXC_PATH, DOMAIN_NAME);
        if (lxc.isDefined()) {
            if (lxc.getState() != LxcDomain::State::STOPPED) {
                lxc.stop();
            }
            lxc.destroy();
        }
    }

    void waitForInit()
    {
        // wait for init fully started (wait for bash to be able to trap SIGTERM)
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
};

} // namespace

BOOST_FIXTURE_TEST_SUITE(LxcDomainSuite, Fixture)

BOOST_AUTO_TEST_CASE(ConstructorDestructorTest)
{
    LxcDomain lxc(LXC_PATH, DOMAIN_NAME);
}

BOOST_AUTO_TEST_CASE(CreateDestroyTest)
{
    LxcDomain lxc(LXC_PATH, DOMAIN_NAME);
    BOOST_CHECK(!lxc.isDefined());

    BOOST_CHECK(lxc.create(TEMPLATE, TEMPLATE_ARGS));

    BOOST_CHECK(lxc.isDefined());
    BOOST_CHECK_EQUAL(lxc.getConfigItem("lxc.rootfs"), LXC_PATH + DOMAIN_NAME + "/rootfs");
    BOOST_CHECK_THROW(lxc.getConfigItem("xxx"), LxcException);

    BOOST_CHECK(lxc.destroy());

    BOOST_CHECK(!lxc.isDefined());
}

BOOST_AUTO_TEST_CASE(StartShutdownTest)
{
    {
        LxcDomain lxc(LXC_PATH, DOMAIN_NAME);
        BOOST_CHECK(lxc.create(TEMPLATE, TEMPLATE_ARGS));
    }
    LxcDomain lxc(LXC_PATH, DOMAIN_NAME);
    BOOST_CHECK(lxc.getState() == LxcDomain::State::STOPPED);
    const char* argv[] = {
        "/bin/sh",
        "-c",
        "trap exit SIGTERM; read",
        NULL
    };
    BOOST_CHECK(lxc.start(argv));
    BOOST_CHECK(lxc.getState() == LxcDomain::State::RUNNING);
    waitForInit();
    BOOST_CHECK(lxc.shutdown(2));
    BOOST_CHECK(lxc.getState() == LxcDomain::State::STOPPED);

    BOOST_CHECK(lxc.destroy());
}

BOOST_AUTO_TEST_CASE(StartStopTest)
{
    {
        LxcDomain lxc(LXC_PATH, DOMAIN_NAME);
        BOOST_CHECK(lxc.create(TEMPLATE, TEMPLATE_ARGS));
    }
    LxcDomain lxc(LXC_PATH, DOMAIN_NAME);
    BOOST_CHECK(lxc.getState() == LxcDomain::State::STOPPED);
    const char* argv[] = {
        "/bin/sh",
        NULL
    };
    BOOST_CHECK(lxc.start(argv));
    BOOST_CHECK(lxc.getState() == LxcDomain::State::RUNNING);
    BOOST_CHECK(!lxc.shutdown(1));
    BOOST_CHECK(lxc.getState() == LxcDomain::State::RUNNING);
    BOOST_CHECK(lxc.stop());
    BOOST_CHECK(lxc.getState() == LxcDomain::State::STOPPED);

    BOOST_CHECK(lxc.destroy());
}

BOOST_AUTO_TEST_CASE(StartHasStoppedTest)
{
    {
        LxcDomain lxc(LXC_PATH, DOMAIN_NAME);
        BOOST_CHECK(lxc.create(TEMPLATE, TEMPLATE_ARGS));
    }
    LxcDomain lxc(LXC_PATH, DOMAIN_NAME);
    BOOST_CHECK(lxc.getState() == LxcDomain::State::STOPPED);
    const char* argv[] = {
        "/bin/sh",
        "-c",
        "echo",
        NULL
    };
    BOOST_CHECK(lxc.start(argv));
    waitForInit();
    BOOST_CHECK(lxc.getState() == LxcDomain::State::STOPPED);

    BOOST_CHECK(lxc.destroy());
}

BOOST_AUTO_TEST_CASE(FreezeUnfreezeTest)
{
    LxcDomain lxc(LXC_PATH, DOMAIN_NAME);
    BOOST_CHECK(lxc.create(TEMPLATE, TEMPLATE_ARGS));
    const char* argv[] = {
        "/bin/sh",
        "-c",
        "trap exit SIGTERM; read",
        NULL
    };
    BOOST_CHECK(lxc.start(argv));
    BOOST_CHECK(lxc.getState() == LxcDomain::State::RUNNING);
    waitForInit();
    BOOST_CHECK(lxc.freeze());
    BOOST_CHECK(lxc.getState() == LxcDomain::State::FROZEN);
    BOOST_CHECK(lxc.unfreeze());
    BOOST_CHECK(lxc.getState() == LxcDomain::State::RUNNING);
    BOOST_CHECK(lxc.shutdown(2));
    BOOST_CHECK(lxc.getState() == LxcDomain::State::STOPPED);

    BOOST_CHECK(lxc.destroy());
}

BOOST_AUTO_TEST_CASE(FreezeStopTest)
{
    LxcDomain lxc(LXC_PATH, DOMAIN_NAME);
    BOOST_CHECK(lxc.create(TEMPLATE, TEMPLATE_ARGS));
    const char* argv[] = {
        "/bin/sh",
        "-c",
        "trap exit SIGTERM; read",
        NULL
    };
    BOOST_CHECK(lxc.start(argv));
    BOOST_CHECK(lxc.getState() == LxcDomain::State::RUNNING);
    waitForInit();
    BOOST_CHECK(lxc.freeze());
    BOOST_CHECK(lxc.getState() == LxcDomain::State::FROZEN);
    BOOST_CHECK(!lxc.shutdown(1));
    BOOST_CHECK(lxc.getState() == LxcDomain::State::FROZEN);
    BOOST_CHECK(lxc.stop());
    BOOST_CHECK(lxc.getState() == LxcDomain::State::STOPPED);

    BOOST_CHECK(lxc.destroy());
}

BOOST_AUTO_TEST_CASE(RepeatTest)
{
    LxcDomain lxc(LXC_PATH, DOMAIN_NAME);
    BOOST_CHECK(lxc.create(TEMPLATE, TEMPLATE_ARGS));
    BOOST_CHECK(!lxc.create(TEMPLATE, TEMPLATE_ARGS));// forbidden
    const char* argv[] = {
        "/bin/sh",
        "-c",
        "trap exit SIGTERM; read",
        NULL
    };
    BOOST_CHECK(lxc.start(argv));
    BOOST_CHECK(lxc.getState() == LxcDomain::State::RUNNING);
    waitForInit();
    BOOST_CHECK(!lxc.start(argv)); // forbidden
    BOOST_CHECK(lxc.freeze());
    BOOST_CHECK(lxc.getState() == LxcDomain::State::FROZEN);
    BOOST_CHECK(lxc.freeze()); // repeat is nop
    BOOST_CHECK(lxc.getState() == LxcDomain::State::FROZEN);
    BOOST_CHECK(lxc.unfreeze());
    BOOST_CHECK(lxc.getState() == LxcDomain::State::RUNNING);
    BOOST_CHECK(lxc.unfreeze()); // repeat is nop
    BOOST_CHECK(lxc.getState() == LxcDomain::State::RUNNING);
    BOOST_CHECK(lxc.stop());
    BOOST_CHECK(lxc.getState() == LxcDomain::State::STOPPED);
    BOOST_CHECK(lxc.stop()); // repeat is nop
    BOOST_CHECK(lxc.getState() == LxcDomain::State::STOPPED);

    BOOST_CHECK(lxc.destroy());
    BOOST_CHECK(!lxc.isDefined());
    BOOST_CHECK(!lxc.destroy()); // forbidden (why?)
}

BOOST_AUTO_TEST_SUITE_END()
