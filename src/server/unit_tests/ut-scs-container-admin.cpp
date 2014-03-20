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
 * @file    ut-scs-container-admin.cpp
 * @author  Jan Olszak (j.olszak@samsung.com)
 * @brief   Unit tests of the ContainerAdmin class
 */

#include "ut.hpp"
#include "scs-container-admin.hpp"
#include "scs-exception.hpp"

#include <memory>
#include <string>


BOOST_AUTO_TEST_SUITE(ContainerAdminSuite)

using namespace security_containers;

const std::string TEST_CONFIG_PATH = "/etc/security-containers/config/tests/ut-scs-container-manager/libvirt-config/test.xml";
const std::string BUGGY_CONFIG_PATH = "/etc/security-containers/config/tests/ut-scs-container-manager/libvirt-config/buggy.xml";
const std::string MISSING_CONFIG_PATH = "/this/is/a/missing/file/path/missing.xml";


BOOST_AUTO_TEST_CASE(ConstructorTest)
{
    BOOST_REQUIRE_NO_THROW(ContainerAdmin ca(TEST_CONFIG_PATH));
}

BOOST_AUTO_TEST_CASE(DestructorTest)
{
    std::unique_ptr<ContainerAdmin> ca(new ContainerAdmin(TEST_CONFIG_PATH));
    BOOST_REQUIRE_NO_THROW(ca.reset());
}

BOOST_AUTO_TEST_CASE(BuggyConfigTest)
{
    BOOST_REQUIRE_THROW(ContainerAdmin ca(BUGGY_CONFIG_PATH), ServerException);
}

BOOST_AUTO_TEST_CASE(MissingConfigTest)
{
    BOOST_REQUIRE_THROW(ContainerAdmin ca(MISSING_CONFIG_PATH), ConfigException);
}

BOOST_AUTO_TEST_CASE(StartTest)
{
    ContainerAdmin ca(TEST_CONFIG_PATH);
    BOOST_REQUIRE_NO_THROW(ca.start());
    BOOST_CHECK(ca.isRunning());
}

BOOST_AUTO_TEST_CASE(StopTest)
{
    ContainerAdmin ca(TEST_CONFIG_PATH);
    BOOST_REQUIRE_NO_THROW(ca.start());
    BOOST_CHECK(ca.isRunning());
    BOOST_REQUIRE_NO_THROW(ca.stop())
    BOOST_CHECK(!ca.isRunning());
    BOOST_CHECK(ca.isStopped());
}

BOOST_AUTO_TEST_CASE(ShutdownTest)
{
    ContainerAdmin ca(TEST_CONFIG_PATH);
    BOOST_REQUIRE_NO_THROW(ca.start())
    BOOST_CHECK(ca.isRunning());
    BOOST_REQUIRE_NO_THROW(ca.shutdown())
    // TODO: For this simple configuration, the shutdown signal is ignored
    // BOOST_CHECK(!ca.isRunning());
    // BOOST_CHECK(ca.isStopped());
}

BOOST_AUTO_TEST_CASE(SuspendTest)
{
    ContainerAdmin ca(TEST_CONFIG_PATH);
    BOOST_REQUIRE_NO_THROW(ca.start())
    BOOST_CHECK(ca.isRunning());
    BOOST_REQUIRE_NO_THROW(ca.suspend())
    BOOST_CHECK(!ca.isRunning());
    BOOST_CHECK(ca.isPaused());
}

BOOST_AUTO_TEST_CASE(ResumeTest)
{
    ContainerAdmin ca(TEST_CONFIG_PATH);
    BOOST_REQUIRE_NO_THROW(ca.start());
    BOOST_REQUIRE_NO_THROW(ca.suspend())
    BOOST_CHECK(ca.isPaused());
    BOOST_REQUIRE_NO_THROW(ca.resume());
    BOOST_CHECK(!ca.isPaused());
    BOOST_CHECK(ca.isRunning());
}

BOOST_AUTO_TEST_SUITE_END()
