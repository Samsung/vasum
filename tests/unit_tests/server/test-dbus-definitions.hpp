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
 * @brief   Common definitions for dbus tests
 */

#ifndef UNIT_TESTS_SERVER_TEST_DBUS_DEFINITIONS_HPP
#define UNIT_TESTS_SERVER_TEST_DBUS_DEFINITIONS_HPP

#include <string>


namespace vasum {
namespace testapi {


const std::string BUS_NAME       = "org.tizen.vasum.tests";
const std::string OBJECT_PATH    = "/org/tizen/vasum/tests";
const std::string INTERFACE      = "tests.api";
const std::string METHOD         = "Method";

const std::string DEFINITION =
    "<node>"
    "  <interface name='" + INTERFACE + "'>"
    "    <method name='" + METHOD + "'>"
    "      <arg type='s' name='argument' direction='in'/>"
    "      <arg type='s' name='response' direction='out'/>"
    "    </method>"
    "  </interface>"
    "</node>";


} // namespace testapi
} // namespace vasum


#endif // UNIT_TESTS_SERVER_TEST_DBUS_DEFINITIONS_HPP
