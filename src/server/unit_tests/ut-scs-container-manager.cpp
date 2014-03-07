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
#include "scs-container-manager.hpp"
#include "scs-exception.hpp"

#include <memory>
#include <string>


BOOST_AUTO_TEST_SUITE(ContainerManagerSuite)

using namespace security_containers;

const std::string daemonConfigPath = "/etc/security-containers/config/tests/ut-scs-container-manager/test-daemon.conf";


BOOST_AUTO_TEST_CASE(ConstructorTest)
{
    BOOST_REQUIRE_NO_THROW(ContainerManager cm(daemonConfigPath););
}

BOOST_AUTO_TEST_CASE(DestructorTest)
{
    std::unique_ptr<ContainerManager> cm(new ContainerManager(daemonConfigPath));
    BOOST_REQUIRE_NO_THROW(cm.reset());
}

BOOST_AUTO_TEST_CASE(StartAllTest)
{
    ContainerManager cm(daemonConfigPath);
    BOOST_REQUIRE_NO_THROW(cm.startAll());
    BOOST_CHECK(!cm.getRunningContainerId().empty());
}

BOOST_AUTO_TEST_CASE(StopAllTest)
{
    ContainerManager cm(daemonConfigPath);
    BOOST_REQUIRE_NO_THROW(cm.startAll());
    BOOST_REQUIRE_NO_THROW(cm.stopAll());
    BOOST_CHECK(cm.getRunningContainerId().empty());

}

BOOST_AUTO_TEST_CASE(FocusTest)
{
    ContainerManager cm(daemonConfigPath);
    BOOST_REQUIRE_NO_THROW(cm.startAll());
    BOOST_REQUIRE_NO_THROW(cm.focus("console"));
    BOOST_CHECK(!cm.getSuspendedContainerIds().empty());
    BOOST_TEST_MESSAGE("Suspended");
    for (auto& id : cm.getSuspendedContainerIds()) {
        BOOST_TEST_MESSAGE(id);
    }
}

BOOST_AUTO_TEST_SUITE_END()
