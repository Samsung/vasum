/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Lukasz Pawelczyk <l.pawelczyk@partner.samsung.com>
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
 * @author  Lukasz Pawelczyk (l.pawelczyk@partner.samsung.com)
 * @brief   Unit tests of the LibvirtConnection class
 */

#include "ut.hpp"

#include "libvirt/connection.hpp"
#include "libvirt/exception.hpp"

#include <memory>

BOOST_AUTO_TEST_SUITE(LibvirtConnectionSuite)


using namespace security_containers;
using namespace security_containers::libvirt;


const std::string CORRECT_URI_STRING = LIBVIRT_LXC_ADDRESS;
const std::string BUGGY_URI_STRING = "some_random_string";

BOOST_AUTO_TEST_CASE(ConstructorTest)
{
    std::unique_ptr<LibvirtConnection> con;
    BOOST_REQUIRE_NO_THROW(con.reset(new LibvirtConnection(CORRECT_URI_STRING)));
}

BOOST_AUTO_TEST_CASE(DestructorTest)
{
    std::unique_ptr<LibvirtConnection> con(new LibvirtConnection(CORRECT_URI_STRING));
    BOOST_REQUIRE_NO_THROW(con.reset());
}

BOOST_AUTO_TEST_CASE(BuggyConfigTest)
{
    std::unique_ptr<LibvirtConnection> con;
    BOOST_REQUIRE_THROW(con.reset(new LibvirtConnection(BUGGY_URI_STRING)), LibvirtOperationException);
}

BOOST_AUTO_TEST_CASE(ConnectionTest)
{
    std::unique_ptr<LibvirtConnection> con(new LibvirtConnection(CORRECT_URI_STRING));
    BOOST_CHECK(con->get() != NULL);
}

BOOST_AUTO_TEST_SUITE_END()
