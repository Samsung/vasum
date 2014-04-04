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
 * @brief   Logger
 */

#include "log/logger.hpp"
#include "log/backend-null.hpp"

#include <iomanip>
#include <memory>
#include <mutex>
#include <cassert>


namespace security_containers {
namespace log {


namespace {

volatile LogLevel gLogLevel = LogLevel::DEBUG;
std::unique_ptr<LogBackend> gLogBackendPtr(new NullLogger());
std::mutex gLogMutex;

const std::string SOURCE_DIR = PROJECT_SOURCE_DIR "/";

inline std::string stripProjectDir(const std::string& file)
{
    // it will work until someone use in cmake FILE(GLOB ... RELATIVE ...)
    assert(0 == file.compare(0, SOURCE_DIR.size(), SOURCE_DIR));
    return file.substr(SOURCE_DIR.size());
}

} // namespace

Logger::Logger(const std::string& severity, const std::string& file, const int line)
{
    mLogLine << std::left << std::setw(8) << '[' + severity + ']'
             << std::left << std::setw(40) << stripProjectDir(file) + ':' + std::to_string(line)
             << ' ';
}

void Logger::logMessage(const std::string& message)
{
    mLogLine << message << std::endl;
    std::unique_lock<std::mutex> lock(gLogMutex);
    gLogBackendPtr->log(mLogLine.str());
}

void Logger::setLogLevel(LogLevel level)
{
    gLogLevel = level;
}

LogLevel Logger::getLogLevel(void)
{
    return gLogLevel;
}

void Logger::setLogBackend(LogBackend* pBackend)
{
    std::unique_lock<std::mutex> lock(gLogMutex);
    gLogBackendPtr.reset(pBackend);
}

} // namespace log
} // namespace security_containers
