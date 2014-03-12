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
 * @file    ut-scs-container.cpp
 * @author  Jan Olszak (j.olszak@samsung.com)
 * @brief   Unit tests of the Container class
 */

#include "ut.hpp"
#include "scs-container.hpp"
#include "scs-exception.hpp"

#include <memory>


BOOST_AUTO_TEST_SUITE(ContainerSuite)

using namespace security_containers;

const std::string testConfigXML =
    "<domain type=\"lxc\">                  \
        <name>cnsl</name>                   \
        <memory>102400</memory>             \
        <on_poweroff>destroy</on_poweroff>  \
        <os>                                \
            <type>exe</type>                \
            <init>/bin/sh</init>            \
        </os>                               \
        <devices>                           \
            <console type=\"pty\"/>         \
        </devices>                          \
    </domain>";


BOOST_AUTO_TEST_CASE(ConstructorTest)
{
    BOOST_CHECK_THROW(Container c1("<><TRASHXML>"), ServerException);
    BOOST_CHECK_NO_THROW(Container c2(testConfigXML));
}

BOOST_AUTO_TEST_CASE(DestructorTest)
{
    std::unique_ptr<Container> c(new Container(testConfigXML));
    BOOST_REQUIRE_NO_THROW(c.reset());
}

BOOST_AUTO_TEST_CASE(StartTest)
{
    Container c(testConfigXML);
    BOOST_REQUIRE_NO_THROW(c.start());
    BOOST_CHECK(c.isRunning());
}

BOOST_AUTO_TEST_CASE(StopTest)
{
    Container c(testConfigXML);
    BOOST_REQUIRE_NO_THROW(c.start());
    BOOST_CHECK(c.isRunning());
    BOOST_REQUIRE_NO_THROW(c.stop())
    BOOST_CHECK(!c.isRunning());
    BOOST_CHECK(c.isStopped());
}

BOOST_AUTO_TEST_CASE(ShutdownTest)
{
    Container c(testConfigXML);
    BOOST_REQUIRE_NO_THROW(c.start())
    BOOST_CHECK(c.isRunning());
    BOOST_REQUIRE_NO_THROW(c.shutdown())
    // TODO: For this simple configuration, the shutdown signal is ignored
    // BOOST_CHECK(!c.isRunning());
    // BOOST_CHECK(c.isStopped());
}

BOOST_AUTO_TEST_CASE(SuspendTest)
{
    Container c(testConfigXML);
    BOOST_REQUIRE_NO_THROW(c.start())
    BOOST_CHECK(c.isRunning());
    BOOST_REQUIRE_NO_THROW(c.suspend())
    BOOST_CHECK(!c.isRunning());
    BOOST_CHECK(c.isPaused());
}

BOOST_AUTO_TEST_CASE(ResumeTest)
{
    Container c(testConfigXML);
    BOOST_REQUIRE_NO_THROW(c.start());
    BOOST_REQUIRE_NO_THROW(c.suspend())
    BOOST_CHECK(c.isPaused());
    BOOST_REQUIRE_NO_THROW(c.resume());
    BOOST_CHECK(!c.isPaused());
    BOOST_CHECK(c.isRunning());
}

BOOST_AUTO_TEST_SUITE_END()
