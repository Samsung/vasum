/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Dariusz Michaluk <d.michaluk@samsung.com>
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
 * @brief   Helper formatter for logger
 */

#ifndef LOGGER_FORMATTER_HPP
#define LOGGER_FORMATTER_HPP

#include "logger/level.hpp"

#include <string>

namespace logger {

class LogFormatter {
public:
    static unsigned int getCurrentThread(void);
    static std::string getCurrentTime(void);
    static std::string getConsoleColor(LogLevel logLevel);
    static std::string getDefaultConsoleColor(void);
    static std::string stripProjectDir(const std::string& file,
                                       const std::string& rootDir);
    static std::string getHeader(LogLevel logLevel,
                                 const std::string& file,
                                 const unsigned int& line,
                                 const std::string& func);
};

} // namespace logger

#endif // LOGGER_FORMATTER_HPP
