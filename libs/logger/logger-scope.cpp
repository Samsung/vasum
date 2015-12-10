/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Lukasz Kostyra <l.kostyra@samsung.com>
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
 * @author  Lukasz Kostyra (l.kostyra@samsung.com)
 * @brief   Scope logger class implementation
 */

#include "config.hpp"

#include "logger/logger-scope.hpp"
#include "logger/logger.hpp"

namespace logger {

SStreamWrapper::operator std::string() const
{
    return mSStream.str();
}

LoggerScope::LoggerScope(const std::string& file,
                         const unsigned int line,
                         const std::string& func,
                         const std::string& message,
                         const std::string& rootDir):
        mFile(file),
        mLine(line),
        mFunc(func),
        mMessage(message),
        mRootDir(rootDir)
{
    if (logger::Logger::getLogLevel() <= logger::LogLevel::TRACE) {
        logger::Logger::logMessage(logger::LogLevel::TRACE, "Entering: " + mMessage,
                                   mFile, mLine, mFunc, mRootDir);
    }
}

LoggerScope::~LoggerScope()
{
    if (logger::Logger::getLogLevel() <= logger::LogLevel::TRACE) {
        logger::Logger::logMessage(logger::LogLevel::TRACE, "Leaving:  " + mMessage,
                                   mFile, mLine, mFunc, mRootDir);
    }
}

} // namespace logger
