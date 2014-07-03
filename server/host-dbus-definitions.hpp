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
 * @brief   Host dbus api definitions
 */

#ifndef SERVER_HOST_DBUS_DEFINITIONS_HPP
#define SERVER_HOST_DBUS_DEFINITIONS_HPP

#include <string>


namespace security_containers {
namespace hostapi {


const std::string BUS_NAME                          = "org.tizen.containers.host";
const std::string OBJECT_PATH                       = "/org/tizen/containers/host";
const std::string INTERFACE                         = "org.tizen.containers.host.test";

const std::string METHOD_TEST                       = "Test";

const std::string DEFINITION =
    "<node>"
    "  <interface name='" + INTERFACE + "'>"
    "    <method name='" + METHOD_TEST + "'>"
    "    </method>"
    "  </interface>"
    "</node>";


} // namespace hostapi
} // namespace security_containers


#endif // SERVER_HOST_DBUS_DEFINITIONS_HPP
