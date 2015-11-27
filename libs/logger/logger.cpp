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

#include "config.hpp"

#include "logger/logger.hpp"
#include "logger/formatter.hpp"
#include "logger/backend-null.hpp"

#include <memory>
#include <mutex>

namespace logger {

namespace {

volatile LogLevel gLogLevel = LogLevel::DEBUG;
std::unique_ptr<LogBackend> gLogBackendPtr(new NullLogger());
std::mutex gLogMutex;

} // namespace

void setupLogger(const LogType type,
                 const LogLevel level,
                 const std::string &arg)
{
    if (type == LogType::LOG_FILE || type == LogType::LOG_PERSISTENT_FILE) {
        if (arg.empty()) {
            throw std::runtime_error("Path needs to be specified in the agument");
        }
    }

    switch(type) {
    case LogType::LOG_NULL:
        Logger::setLogBackend(new NullLogger());
        break;
#ifdef HAVE_SYSTEMD
    case LogType::LOG_JOURNALD:
        Logger::setLogBackend(new SystemdJournalBackend());
        break;
#endif
    case LogType::LOG_FILE:
        Logger::setLogBackend(new FileBackend(arg));
        break;
    case LogType::LOG_PERSISTENT_FILE:
        Logger::setLogBackend(new PersistentFileBackend(arg));
        break;
    case LogType::LOG_SYSLOG:
        Logger::setLogBackend(new SyslogBackend());
        break;
    case LogType::LOG_STDERR:
        Logger::setLogBackend(new StderrBackend());
        break;
    default:
        throw std::runtime_error("Bad logger type passed");
    }

    Logger::setLogLevel(level);
}

void Logger::logMessage(LogLevel logLevel,
                        const std::string& message,
                        const std::string& file,
                        const unsigned int line,
                        const std::string& func,
                        const std::string& rootDir)
{
    std::string sfile = LogFormatter::stripProjectDir(file, rootDir);
    std::unique_lock<std::mutex> lock(gLogMutex);
    gLogBackendPtr->log(logLevel, sfile, line, func, message);
}

void Logger::logRelog(LogLevel logLevel,
                      const std::istream& stream,
                      const std::string& file,
                      const unsigned int line,
                      const std::string& func,
                      const std::string& rootDir)
{
    std::string sfile = LogFormatter::stripProjectDir(file, rootDir);
    std::unique_lock<std::mutex> lock(gLogMutex);
    gLogBackendPtr->relog(logLevel, sfile, line, func, stream);
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

} // namespace logger
