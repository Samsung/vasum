/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Jan Olszak <j.olszak@samsung.com>
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
 * @author  Jan Olszak (j.olszak@samsung.com)
 * @brief   container-daemon dbus api definitions
 */

#ifndef CONTAINER_DAEMON_DAEMON_DBUS_DEFINITIONS_HPP
#define CONTAINER_DAEMON_DAEMON_DBUS_DEFINITIONS_HPP

#include <string>


namespace security_containers {
namespace container_daemon {
namespace api {

const std::string BUS_NAME            = "com.samsung.container.daemon";
const std::string OBJECT_PATH         = "/com/samsung/container/daemon";
const std::string INTERFACE           = "com.samsung.container.daemon";

const std::string METHOD_GAIN_FOCUS   = "GainFocus";
const std::string METHOD_LOSE_FOCUS   = "LoseFocus";

const std::string DEFINITION =
    "<node>"
    "  <interface name='" + INTERFACE + "'>"
    "    <method name='" + METHOD_GAIN_FOCUS + "'>"
    "    </method>"
    "    <method name='" + METHOD_LOSE_FOCUS + "'>"
    "    </method>"
    "  </interface>"
    "</node>";


} // namespace api
} // namespace container_daemon
} // namespace security_containers


#endif // CONTAINER_DAEMON_DAEMON_DBUS_DEFINITIONS_HPP
