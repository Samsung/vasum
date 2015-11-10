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
 * @brief   Scope logger class declaration
 */

#ifndef LOGGER_LOGGER_SCOPE_HPP
#define LOGGER_LOGGER_SCOPE_HPP

#include <string>
#include <sstream>

namespace logger {

class SStreamWrapper
{
public:
    operator std::string() const;

    template <typename T>
    SStreamWrapper& operator<<(const T& b)
    {
        this->mSStream << b;
        return *this;
    }

private:
    std::ostringstream mSStream;
};

/**
 * Class specifically for scope debug logging. Should be used at the beggining of a scope.
 * Constructor marks scope enterance, destructor marks scope leave.
 */
class LoggerScope
{
public:
    LoggerScope(const std::string& file,
                const unsigned int line,
                const std::string& func,
                const std::string& message,
                const std::string& rootDir);
    ~LoggerScope();

private:
    const std::string mFile;
    const unsigned int mLine;
    const std::string mFunc;
    const std::string mMessage;
    const std::string mRootDir;
};

} // namespace logger

/**
 * @brief Automatically create LoggerScope object which logs at the construction and destruction
 * @ingroup libLogger
 */
#if !defined(NDEBUG)
#define LOGS(MSG)   logger::LoggerScope logScopeObj(__FILE__, __LINE__, __func__,    \
                                                    logger::SStreamWrapper() << MSG, \
                                                    PROJECT_SOURCE_DIR)
#else
#define LOGS(MSG) do {} while (0)
#endif

#endif // LOGGER_LOGGER_SCOPE_HPP
