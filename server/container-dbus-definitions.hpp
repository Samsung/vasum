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
 * @brief   Container dbus api definitions
 */

#ifndef SERVER_CONTAINER_DBUS_DEFINITIONS_HPP
#define SERVER_CONTAINER_DBUS_DEFINITIONS_HPP

#include <string>


namespace security_containers {
namespace api {


const std::string BUS_NAME                          = "com.samsung.containers";
const std::string OBJECT_PATH                       = "/com/samsung/containers";
const std::string INTERFACE                         = "com.samsung.containers.manager";

const std::string METHOD_NOTIFY_ACTIVE_CONTAINER    = "NotifyActiveContainer";
const std::string SIGNAL_NOTIFICATION               = "Notification";

const std::string DEFINITION =
    "<node>"
    "  <interface name='" + INTERFACE + "'>"
    "    <method name='" + METHOD_NOTIFY_ACTIVE_CONTAINER + "'>"
    "      <arg type='s' name='application' direction='in'/>"
    "      <arg type='s' name='message' direction='in'/>"
    "    </method>"
    "    <signal name='" + SIGNAL_NOTIFICATION + "'>"
    "      <arg type='s' name='container'/>"
    "      <arg type='s' name='application'/>"
    "      <arg type='s' name='message'/>"
    "    </signal>"
    "  </interface>"
    "</node>";


} // namespace api
} // namespace security_containers


#endif // SERVER_CONTAINER_DBUS_DEFINITIONS_HPP
