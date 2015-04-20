/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Jan Olszak <j.olszak@samsung.com>
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
 * @author  Jan Olszak (j.olszak@samsung.com)
 * @brief   Logger
 */

#ifndef COMMON_LOGGER_LOGGER_HPP
#define COMMON_LOGGER_LOGGER_HPP

#include "logger/level.hpp"

#include <sstream>
#include <string>

#ifndef PROJECT_SOURCE_DIR
#define PROJECT_SOURCE_DIR ""
#endif

namespace logger {

class LogBackend;

class Logger {
public:
    static void logMessage(LogLevel logLevel,
                           const std::string& message,
                           const std::string& file,
                           const unsigned int line,
                           const std::string& func,
                           const std::string& rootDir);

    static void setLogLevel(const LogLevel level);
    static void setLogLevel(const std::string& level);
    static LogLevel getLogLevel(void);
    static void setLogBackend(LogBackend* pBackend);
};

} // namespace logger

#define LOG(SEVERITY, MESSAGE)                                             \
    do {                                                                   \
        if (logger::Logger::getLogLevel() <= logger::LogLevel::SEVERITY) { \
            std::ostringstream messageStream__;                            \
            messageStream__ << MESSAGE;                                    \
            logger::Logger::logMessage(logger::LogLevel::SEVERITY,         \
                                       messageStream__.str(),              \
                                       __FILE__,                           \
                                       __LINE__,                           \
                                       __func__,                           \
                                       PROJECT_SOURCE_DIR);                \
        }                                                                  \
    } while (0)

#define LOGE(MESSAGE) LOG(ERROR, MESSAGE)
#define LOGW(MESSAGE) LOG(WARN, MESSAGE)
#define LOGI(MESSAGE) LOG(INFO, MESSAGE)
#define LOGD(MESSAGE) LOG(DEBUG, MESSAGE)
#define LOGH(MESSAGE) LOG(HELP, MESSAGE)
#define LOGT(MESSAGE) LOG(TRACE, MESSAGE)

#endif // COMMON_LOGGER_LOGGER_HPP
