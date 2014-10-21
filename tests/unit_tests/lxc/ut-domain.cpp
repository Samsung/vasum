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

#include <thread>
#include <chrono>
#include <boost/filesystem.hpp>

namespace {

using namespace security_containers;
using namespace security_containers::lxc;
namespace fs = boost::filesystem;

const std::string LXC_PATH = "/tmp/ut-lxc/";
const std::string DOMAIN_NAME = "ut-domain";
const std::string TEMPLATE = SC_TEST_LXC_TEMPLATES_INSTALL_DIR "/minimal.sh";

struct Fixture {
    Fixture()
    {
        fs::create_directory(LXC_PATH);
        cleanup();
    }

    ~Fixture()
    {
        cleanup();
        fs::remove_all(LXC_PATH);
    }

    void cleanup()
    {
        LxcDomain lxc(LXC_PATH, DOMAIN_NAME);
        if (lxc.isDefined()) {
            if (lxc.isRunning()) {
                lxc.stop();
            }
            lxc.destroy();
        }
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

    lxc.create(TEMPLATE);

    BOOST_CHECK(lxc.isDefined());
    BOOST_CHECK_EQUAL(lxc.getConfigItem("lxc.rootfs"), LXC_PATH + DOMAIN_NAME + "/rootfs");
    BOOST_CHECK_THROW(lxc.getConfigItem("xxx"), LxcException);

    lxc.destroy();

    BOOST_CHECK(!lxc.isDefined());
}

BOOST_AUTO_TEST_CASE(StartShutdownTest)
{
    {
        LxcDomain lxc(LXC_PATH, DOMAIN_NAME);
        lxc.create(TEMPLATE);
    }
    LxcDomain lxc(LXC_PATH, DOMAIN_NAME);
    BOOST_CHECK_EQUAL("STOPPED", lxc.getState());
    const char* argv[] = {
        "/bin/sh",
        "-c",
        "trap exit SIGTERM; read",
        NULL
    };
    lxc.start(argv);
    // wait for bash to be able to trap SIGTERM
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    BOOST_CHECK_EQUAL("RUNNING", lxc.getState());
    lxc.shutdown(2);
    BOOST_CHECK_EQUAL("STOPPED", lxc.getState());

    lxc.destroy();
}

BOOST_AUTO_TEST_CASE(StartStopTest)
{
    {
        LxcDomain lxc(LXC_PATH, DOMAIN_NAME);
        lxc.create(TEMPLATE);
    }
    LxcDomain lxc(LXC_PATH, DOMAIN_NAME);
    BOOST_CHECK_EQUAL("STOPPED", lxc.getState());
    const char* argv[] = {
        "/bin/sh",
        NULL
    };
    lxc.start(argv);
    BOOST_CHECK_EQUAL("RUNNING", lxc.getState());
    BOOST_CHECK_THROW(lxc.shutdown(1), LxcException);
    BOOST_CHECK_EQUAL("RUNNING", lxc.getState());
    lxc.stop();
    BOOST_CHECK_EQUAL("STOPPED", lxc.getState());

    lxc.destroy();
}

BOOST_AUTO_TEST_SUITE_END()
