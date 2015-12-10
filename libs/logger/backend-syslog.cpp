/*
 *  Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Roman Kubiak (r.kubiak@samsung.com)
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
 * @author  Roman Kubiak (r.kubiak@samsung.com)
 * @brief   Syslog backend for logger
 */

#include "config.hpp"

#include "logger/formatter.hpp"
#include "logger/backend-syslog.hpp"

#include <syslog.h>
#include <sstream>
namespace logger {

namespace {

inline int toSyslogPriority(LogLevel logLevel)
{
    switch (logLevel) {
    case LogLevel::ERROR:
        return LOG_ERR;     // 3
    case LogLevel::WARN:
        return LOG_WARNING; // 4
    case LogLevel::INFO:
        return LOG_INFO;    // 6
    case LogLevel::DEBUG:
        return LOG_DEBUG;   // 7
    case LogLevel::TRACE:
        return LOG_DEBUG;   // 7
    case LogLevel::HELP:
        return LOG_DEBUG;   // 7
    default:
        return LOG_DEBUG;   // 7
    }
}

} // namespace

void SyslogBackend::log(LogLevel logLevel,
                        const std::string& file,
                        const unsigned int& line,
                        const std::string& func,
                        const std::string& message)
{
    ::syslog(toSyslogPriority(logLevel), "%s %s", LogFormatter::getHeader(logLevel, file, line, func).c_str(), message.c_str());
}

} // namespace logger
