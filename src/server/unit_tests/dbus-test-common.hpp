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
 * @file    dbus-test-common.hpp
 * @author  Piotr Bartosiewicz (p.bartosiewi@partner.samsung.com)
 * @brief   Common definitions for dbus tests
 */

#ifndef DBUS_TEST_COMMON_HPP
#define DBUS_TEST_COMMON_HPP

#include <string>

const std::string DBUS_SOCKET_FILE       = "/tmp/container_socket";
const std::string DBUS_ADDRESS           = "unix:path=" + DBUS_SOCKET_FILE;

const std::string TESTAPI_BUS_NAME       = "com.samsung.tests";
const std::string TESTAPI_OBJECT_PATH    = "/com/samsung/tests";
const std::string TESTAPI_INTERFACE      = "tests.api";
const std::string TESTAPI_METHOD_NOOP    = "Noop";
const std::string TESTAPI_METHOD_PROCESS = "Process";
const std::string TESTAPI_METHOD_THROW   = "Throw";

const std::string TESTAPI_DEFINITION =
    "<node>"
    "  <interface name='" + TESTAPI_INTERFACE + "'>"
    "    <method name='" + TESTAPI_METHOD_NOOP + "'/>"
    "    <method name='" + TESTAPI_METHOD_PROCESS + "'>"
    "      <arg type='s' name='argument' direction='in'/>"
    "      <arg type='s' name='response' direction='out'/>"
    "    </method>"
    "    <method name='" + TESTAPI_METHOD_THROW + "'>"
    "      <arg type='i' name='argument' direction='in'/>"
    "    </method>"
    "  </interface>"
    "</node>";

#endif //DBUS_TEST_COMMON_HPP
