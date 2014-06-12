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
 * @brief   Unit tests of the NetworkAdmin class
 */

#include "config.hpp"
#include "ut.hpp"

#include "network-admin.hpp"

#include "utils/exception.hpp"
#include "libvirt/exception.hpp"
#include "config/manager.hpp"


using namespace security_containers;

namespace {

const std::string TEST_CONFIG_PATH = SC_TEST_CONFIG_INSTALL_DIR "/server/ut-network-admin/containers/test.conf";
const std::string BUGGY_CONFIG_PATH = SC_TEST_CONFIG_INSTALL_DIR "/server/ut-network-admin/containers/buggy.conf";
const std::string MISSING_CONFIG_PATH = SC_TEST_CONFIG_INSTALL_DIR "/server/ut-network-admin/containers/missing.conf";

} // namespace


BOOST_AUTO_TEST_SUITE(NetworkAdminSuite)

BOOST_AUTO_TEST_CASE(ConstructorDestructorTest)
{
    ContainerConfig config;
    config::loadFromFile(TEST_CONFIG_PATH, config);
    std::unique_ptr<NetworkAdmin> admin;
    BOOST_REQUIRE_NO_THROW(admin.reset(new NetworkAdmin(config)));
    BOOST_REQUIRE_NO_THROW(admin.reset());
}

BOOST_AUTO_TEST_CASE(BuggyConfigTest)
{
    ContainerConfig config;
    config::loadFromFile(BUGGY_CONFIG_PATH, config);
    BOOST_REQUIRE_THROW(NetworkAdmin na(config), LibvirtOperationException);
}

BOOST_AUTO_TEST_CASE(MissingConfigTest)
{
    ContainerConfig config;
    config::loadFromFile(MISSING_CONFIG_PATH, config);
    BOOST_REQUIRE_THROW(NetworkAdmin na(config), UtilsException);
}

BOOST_AUTO_TEST_CASE(StartStopTest)
{
    ContainerConfig config;
    config::loadFromFile(TEST_CONFIG_PATH, config);
    NetworkAdmin net(config);

    BOOST_CHECK(!net.isActive());
    BOOST_CHECK_NO_THROW(net.start());
    BOOST_CHECK(net.isActive());
    BOOST_CHECK_NO_THROW(net.stop());
    BOOST_CHECK(!net.isActive());
}

BOOST_AUTO_TEST_SUITE_END()
