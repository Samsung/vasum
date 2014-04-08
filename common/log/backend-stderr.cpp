/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Pawel Broda <p.broda@partner.samsung.com>
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
 * @author  Pawel Broda (p.broda@partner.samsung.com)
 * @brief   Stderr backend for logger
 */

#include "log/backend-stderr.hpp"

#include <sstream>
#include <iomanip>
#include <sys/time.h>

namespace security_containers {
namespace log {

namespace {

inline std::string getCurrentTime(void)
{
    char time[13];
    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm* tm = localtime(&tv.tv_sec);
    sprintf(time, "%02d:%02d:%02d.%03d", tm->tm_hour, tm->tm_min, tm->tm_sec, int(tv.tv_usec / 1000));

    return std::string(time);
}

} // namespace

void StderrBackend::log(const std::string& severity,
                        const std::string& file,
                        const unsigned int& line,
                        const std::string& func,
                        const std::string& message)
{
    std::ostringstream logLine;

    // example log string
    // 06:52:35.123 [ERROR] src/util/fs.cpp:43 readFileContent: /file/file.txt is missing

    logLine << getCurrentTime() << ' '
            << std::left << std::setw(8) << '[' + severity + ']'
            << std::left << std::setw(52) << file + ':' + std::to_string(line) + ' ' + func + ':'
            << message << std::endl;
    fprintf(stderr, "%s", logLine.str().c_str());
}


} // namespace log
} // namespace security_containers

