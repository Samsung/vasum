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

#include "log/backend-journal.hpp"
#include "log/formatter.hpp"

#include <systemd/sd-journal.h>

namespace security_containers {
namespace log {

void SystemdJournalBackend::log(LogLevel logLevel,
                                const std::string& file,
                                const unsigned int& line,
                                const std::string& func,
                                const std::string& message)
{
#define SD_JOURNAL_SUPPRESS_LOCATION
    sd_journal_send("PRIORITY=%s", toString(logLevel).c_str(),
                    "CODE_FILE=%s", file.c_str(),
                    "CODE_LINE=%d", line,
                    "CODE_FUNC=%s", func.c_str(),
                    "MESSAGE=%s", message.c_str(),
                    NULL);
#undef SD_JOURNAL_SUPPRESS_LOCATION
}

} // namespace log
} // namespace security_containers

