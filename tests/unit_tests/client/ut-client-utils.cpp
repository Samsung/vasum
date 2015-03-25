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
 * @brief   Unit tests of the client utils
 */

#include <config.hpp>
#include "ut.hpp"
#include <utils.hpp>


BOOST_AUTO_TEST_SUITE(ClientUtils)

BOOST_AUTO_TEST_CASE(ParseZoneIdFromCpuSet)
{
    auto testBad = [](const std::string& input) {
        std::string ret;
        BOOST_CHECK(!parseZoneIdFromCpuSet(input, ret));
    };

    auto testOK = [](const std::string& input, const std::string& expected) {
        std::string ret;
        BOOST_CHECK(parseZoneIdFromCpuSet(input, ret));
        BOOST_CHECK_EQUAL(expected, ret);
    };

    testBad("");
    testBad("/foo");

    testOK("/", "host");
    testOK("/machine/a-b.libvirt-lxc", "a-b");
    testOK("/machine.slice/machine-lxc\\x2da\\x2db.scope", "a-b");
    testOK("/machine.slice/machine-lxc\\x2da-b.scope", "a/b");

    testOK("/lxc/test", "test");
}

BOOST_AUTO_TEST_SUITE_END()
