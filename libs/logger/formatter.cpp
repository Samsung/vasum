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

#include "config.hpp"

#include "logger/formatter.hpp"
#include "utils/ccolor.hpp"

#include <unistd.h>
#include <sys/time.h>
#include <cassert>
#include <sstream>
#include <iomanip>
#include <thread>
#include <atomic>

namespace logger {

namespace {

const int TIME_COLUMN_LENGTH = 12;
const int SEVERITY_COLUMN_LENGTH = 8;
const int PID_COLUMN_LENGTH = 8;
const int TID_COLUMN_LENGTH = 2;
const int FILE_COLUMN_LENGTH = 60;

std::atomic<unsigned int> gNextThreadId(1);
thread_local unsigned int gThisThreadId(0);

} // namespace

unsigned int LogFormatter::getCurrentThread(void)
{
    unsigned int id = gThisThreadId;
    if (id == 0) {
        gThisThreadId = id = gNextThreadId++;
    }

    return id;
}

std::string LogFormatter::getCurrentTime(void)
{
    char time[TIME_COLUMN_LENGTH + 1];
    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm* tm = localtime(&tv.tv_sec);
    snprintf(time,
             sizeof(time),
             "%02d:%02d:%02d.%03d",
             tm->tm_hour,
             tm->tm_min,
             tm->tm_sec,
             int(tv.tv_usec / 1000));

    return std::string(time);
}

std::string LogFormatter::getConsoleColor(LogLevel logLevel)
{
    switch (logLevel) {
    case LogLevel::ERROR:
        return utils::getConsoleEscapeSequence(utils::Attributes::BOLD, utils::Color::RED);
    case LogLevel::WARN:
        return utils::getConsoleEscapeSequence(utils::Attributes::BOLD, utils::Color::YELLOW);
    case LogLevel::INFO:
        return utils::getConsoleEscapeSequence(utils::Attributes::BOLD, utils::Color::BLUE);
    case LogLevel::DEBUG:
        return utils::getConsoleEscapeSequence(utils::Attributes::DEFAULT, utils::Color::GREEN);
    case LogLevel::TRACE:
        return utils::getConsoleEscapeSequence(utils::Attributes::DEFAULT, utils::Color::BLACK);
    case LogLevel::HELP:
        return utils::getConsoleEscapeSequence(utils::Attributes::BOLD, utils::Color::MAGENTA);
    default:
        return utils::getConsoleEscapeSequence(utils::Attributes::DEFAULT, utils::Color::DEFAULT);
    }
}

std::string LogFormatter::getDefaultConsoleColor(void)
{
    return utils::getConsoleEscapeSequence(utils::Attributes::DEFAULT, utils::Color::DEFAULT);
}

std::string LogFormatter::stripProjectDir(const std::string& file,
                                          const std::string& rootDir)
{
    // If rootdir is empty then return full name
    if (rootDir.empty()) {
        return file;
    }
    const std::string sourceDir = rootDir + "/";
    // If file does not belong to rootDir then also return full name
    if (0 != file.compare(0, sourceDir.size(), sourceDir)) {
        return file;
    }
    return file.substr(sourceDir.size());
}

std::string LogFormatter::getHeader(LogLevel logLevel,
                                    const std::string& file,
                                    const unsigned int& line,
                                    const std::string& func)
{
    std::ostringstream logLine;
    logLine << getCurrentTime() << ' '
            << std::left << std::setw(SEVERITY_COLUMN_LENGTH) << '[' + toString(logLevel) + ']'
            << std::right << std::setw(PID_COLUMN_LENGTH) << ::getpid() << "/"
            << std::right << std::setw(TID_COLUMN_LENGTH) << getCurrentThread() << ": "
            << std::left << std::setw(FILE_COLUMN_LENGTH)
            << file + ':' + std::to_string(line) + ' ' + func + ':';
    return logLine.str();
}

} // namespace logger
