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
 * @brief   Common dbus api definitions
 */

#ifndef SERVER_COMMON_DBUS_DEFINITIONS_HPP
#define SERVER_COMMON_DBUS_DEFINITIONS_HPP

#include <string>


namespace security_containers {
namespace api {
const std::string ERROR_FORBIDDEN  = "org.tizen.containers.Error.Forbidden";
const std::string ERROR_FORWARDED  = "org.tizen.containers.Error.Forwarded";
const std::string ERROR_INVALID_ID = "org.tizen.containers.Error.InvalidId";
const std::string ERROR_INTERNAL   = "org.tizen.containers.Error.Internal";

const std::string METHOD_PROXY_CALL    = "ProxyCall";

} // namespace api
} // namespace security_containers


#endif // SERVER_COMMON_DBUS_DEFINITIONS_HPP
