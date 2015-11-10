/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Dariusz Michaluk (d.michaluk@samsung.com)
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
 * @author  Dariusz Michaluk (d.michaluk@samsung.com)
 * @brief   LogLevel
 */

#ifndef LOGGER_LEVEL_HPP
#define LOGGER_LEVEL_HPP

#include <string>

namespace logger {

/**
 * @brief Available log levels
 * @ingroup libLogger
 */
enum class LogLevel : int {
    TRACE, ///< Most detailed log level
    DEBUG, ///< Debug logs
    INFO,  ///< Information
    WARN,  ///< Warnings
    ERROR, ///< Errors
    HELP   ///< Helper logs
};

/**
 * @param logLevel LogLevel
 * @return std::sting representation of the LogLevel value
 */
std::string toString(const LogLevel logLevel);

/**
 * @param level string representation of log level
 * @return parsed LogLevel value
 */
LogLevel parseLogLevel(const std::string& level);

} // namespace logger

#endif // LOGGER_LEVEL_HPP
