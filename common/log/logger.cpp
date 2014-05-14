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
#include "log/formatter.hpp"
#include "log/backend-null.hpp"

#include <memory>
#include <mutex>

namespace security_containers {
namespace log {


namespace {

volatile LogLevel gLogLevel = LogLevel::DEBUG;
std::unique_ptr<LogBackend> gLogBackendPtr(new NullLogger());
std::mutex gLogMutex;

} // namespace

Logger::Logger(LogLevel logLevel,
               const std::string& file,
               const unsigned int line,
               const std::string& func)
    : mLogLevel(logLevel),
      mFile(LogFormatter::stripProjectDir(file)),
      mLine(line),
      mFunc(func)
{

}

void Logger::logMessage(const std::string& message)
{
    std::unique_lock<std::mutex> lock(gLogMutex);
    gLogBackendPtr->log(mLogLevel, mFile, mLine, mFunc, message);
}

void Logger::setLogLevel(const LogLevel level)
{
    gLogLevel = level;
}

void Logger::setLogLevel(const std::string& level)
{
    gLogLevel = parseLogLevel(level);
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

