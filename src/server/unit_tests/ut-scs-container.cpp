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
 * @author  Lukasz Pawelczyk (l.pawelczyk@partner.samsung.com)
 * @brief   Unit tests of the Container class
 */

#include "ut.hpp"
#include "scs-container.hpp"
#include "scs-exception.hpp"

#include <memory>
#include <string>


BOOST_AUTO_TEST_SUITE(ContainerSuite)

using namespace security_containers;

const std::string TEST_CONFIG_PATH = "/etc/security-containers/tests/ut-scs-container/containers/test.conf";
const std::string BUGGY_CONFIG_PATH = "/etc/security-containers/tests/ut-scs-container/containers/buggy.conf";
const std::string MISSING_CONFIG_PATH = "/this/is/a/missing/file/path/config.conf";


BOOST_AUTO_TEST_CASE(ConstructorTest)
{
    BOOST_REQUIRE_NO_THROW(Container c(TEST_CONFIG_PATH));
}

BOOST_AUTO_TEST_CASE(DestructorTest)
{
    std::unique_ptr<Container> c(new Container(TEST_CONFIG_PATH));
    BOOST_REQUIRE_NO_THROW(c.reset());
}

BOOST_AUTO_TEST_CASE(BuggyConfigTest)
{
    BOOST_REQUIRE_THROW(Container c(BUGGY_CONFIG_PATH), ServerException);
}

BOOST_AUTO_TEST_CASE(MissingConfigTest)
{
    BOOST_REQUIRE_THROW(Container c(MISSING_CONFIG_PATH), ConfigException);
}


BOOST_AUTO_TEST_SUITE_END()
