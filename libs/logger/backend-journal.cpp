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
 * @brief   Systemd journal backend for logger
 */

#ifdef HAVE_SYSTEMD
#include "config.hpp"

#include "logger/backend-journal.hpp"

#define SD_JOURNAL_SUPPRESS_LOCATION
#include <systemd/sd-journal.h>

namespace logger {

namespace {

inline int toJournalPriority(LogLevel logLevel)
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

void SystemdJournalBackend::log(LogLevel logLevel,
                                const std::string& file,
                                const unsigned int& line,
                                const std::string& func,
                                const std::string& message)
{
    sd_journal_send("PRIORITY=%d", toJournalPriority(logLevel),
                    "CODE_FILE=%s", file.c_str(),
                    "CODE_LINE=%d", line,
                    "CODE_FUNC=%s", func.c_str(),
                    "MESSAGE=%s", message.c_str(),
                    NULL);
}

} // namespace logger
#endif // HAVE_SYSTEMD
