/*
 *  Copyright (c) 2000 - 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Bumjin Im <bj.im@samsung.com>
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
 * @file    log.hpp
 * @author  Jan Olszak (j.olszak@samsung.com)
 * @brief   Logger
 */


#ifndef LOG_HPP
#define LOG_HPP

#include "log-backend.hpp"

#include <sstream>
#include <string.h>

namespace security_containers {
namespace log {

enum class LogLevel {
    TRACE, DEBUG, INFO, WARN, ERROR
};

class Logger {
public:
    Logger(const std::string& severity, const std::string& file, const int line);
    void logMessage(const std::string& message);

    static void setLogLevel(LogLevel level);
    static LogLevel getLogLevel(void);
    static void setLogBackend(LogBackend* pBackend);

private:
    std::ostringstream mLogLine;
};

} // namespace log
} // namespace security_containers


#define BASE_FILE (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define LOG(SEVERITY, MESSAGE) \
    do { \
        if (security_containers::log::Logger::getLogLevel() <= \
            security_containers::log::LogLevel::SEVERITY) { \
            std::ostringstream message; \
            message << MESSAGE; \
            security_containers::log::Logger logger(#SEVERITY, BASE_FILE, __LINE__); \
            logger.logMessage(message.str()); \
        } \
    } while(0)

#define LOGE(MESSAGE) LOG(ERROR, MESSAGE)
#define LOGW(MESSAGE) LOG(WARN, MESSAGE)
#define LOGI(MESSAGE) LOG(INFO, MESSAGE)
#define LOGD(MESSAGE) LOG(DEBUG, MESSAGE)
#define LOGT(MESSAGE) LOG(TRACE, MESSAGE)


#endif // LOG_HPP
