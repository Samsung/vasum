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
 * @brief   Exceptions for the configuration
 */

#ifndef COMMON_CONFIG_EXCEPTION_HPP
#define COMMON_CONFIG_EXCEPTION_HPP

#include <stdexcept>

namespace config {

/**
 * Base class for exceptions in configuration.
 * Error occured during a config file parsing,
 * e.g. syntax error
 */
struct ConfigException: public std::runtime_error {

    ConfigException(const std::string& error) : std::runtime_error(error) {}
};

} // namespace config

#endif // COMMON_CONFIG_EXCEPTION_HPP
