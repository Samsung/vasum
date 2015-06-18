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
 * @brief   Common (dbus, IPC) api definitions
 */

#ifndef SERVER_COMMON_DEFINITIONS_HPP
#define SERVER_COMMON_DEFINITIONS_HPP

#include <string>


namespace vasum {
namespace api {

/**
 * Error codes that can be set in API handlers
 */
//TODO: Errors should use exception handling mechanism
///@{
const std::string ERROR_FORBIDDEN          = "org.tizen.vasum.Error.Forbidden";
const std::string ERROR_FORWARDED          = "org.tizen.vasum.Error.Forwarded";
const std::string ERROR_INVALID_ID         = "org.tizen.vasum.Error.InvalidId";
const std::string ERROR_INVALID_STATE      = "org.tizen.vasum.Error.InvalidState";
const std::string ERROR_INTERNAL           = "org.tizen.vasum.Error.Internal";
const std::string ERROR_ZONE_NOT_RUNNING   = "org.tizen.vasum.Error.ZonesNotRunning";
const std::string ERROR_CREATE_FILE_FAILED = "org.tizen.vasum.Error.CreateFileFailed";
const std::string ERROR_QUEUE              = "org.tizen.vasum.Error.Queue";
///@}

} // namespace api
} // namespace vasum


#endif // SERVER_COMMON_DEFINITIONS_HPP
