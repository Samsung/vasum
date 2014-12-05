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
 * @brief   Declaration of the struct for storing proxy call configuration
 */


#ifndef SERVER_PROXY_CALL_CONFIG_HPP
#define SERVER_PROXY_CALL_CONFIG_HPP


#include "config/fields.hpp"

#include <string>


namespace vasum {

/**
 * A single allow rule for proxy call dispatching.
 */
struct ProxyCallRule {

    std::string caller; ///< caller id (container id or host)
    std::string target; ///< target id (container id or host)
    std::string targetBusName; ///< target dbus bus name
    std::string targetObjectPath; ///< target dbus object path
    std::string targetInterface; ///< target dbus interface
    std::string targetMethod; ///< target dbus method

    CONFIG_REGISTER
    (
        caller,
        target,
        targetBusName,
        targetObjectPath,
        targetInterface,
        targetMethod
    )

};

} // namespace vasum

#endif /* SERVER_PROXY_CALL_CONFIG_HPP */
