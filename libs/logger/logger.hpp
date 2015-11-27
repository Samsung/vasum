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
 * @author   Jan Olszak (j.olszak@samsung.com)
 * @defgroup libLogger libLogger
 * @brief C++ library for handling logging.
 *
 * There are few backends implemented and it's possible to create your own by inheriting after the @ref logger::LogBackend interface.
 *
 * Example usage:
 * @code
 * using namespace logger;
 *
 * // Set minimal logging level
 * Logger::setLogLevel("TRACE");
 *
 *
 * // Set one of the possible backends:
 * Logger::setLogBackend(new NullLogger());
 * Logger::setLogBackend(new SystemdJournalBackend());
 * Logger::setLogBackend(new FileBackend("/tmp/logs.txt"));
 * Logger::setLogBackend(new PersistentFileBackend("/tmp/logs.txt"));
 * Logger::setLogBackend(new SyslogBackend());
 * Logger::setLogBackend(new StderrBackend());
 *
 *
 * // All logs should be visible:
 * LOGE("Error");
 * LOGW("Warning");
 * LOGI("Information");
 * LOGD("Debug");
 * LOGT("Trace");
 * LOGH("Helper");
 *
 * {
 *     LOGS("Scope");
 * }
 *
 * @endcode
 */


#ifndef LOGGER_LOGGER_HPP
#define LOGGER_LOGGER_HPP

#include "logger/level.hpp"
#include "logger/backend-null.hpp"
#ifdef HAVE_SYSTEMD
#include "logger/backend-journal.hpp"
#endif
#include "logger/backend-file.hpp"
#include "logger/backend-persistent-file.hpp"
#include "logger/backend-syslog.hpp"
#include "logger/backend-stderr.hpp"

#include <sstream>
#include <string>

#ifndef PROJECT_SOURCE_DIR
#define PROJECT_SOURCE_DIR ""
#endif

namespace logger {

enum class LogType : int {
    LOG_NULL,
    LOG_JOURNALD,
    LOG_FILE,
    LOG_PERSISTENT_FILE,
    LOG_SYSLOG,
    LOG_STDERR
};

/**
 * A helper function to easily and completely setup a new logger
 *
 * @param type      logger type to be set up
 * @param level     maximum log level that will be logged
 * @param arg       an argument used by some loggers, specific to them
 *                  (e.g. file name for file loggers)
 */
void setupLogger(const LogType type,
                 const LogLevel level,
                 const std::string &arg = "");

class LogBackend;

class Logger {
public:
    static void logMessage(LogLevel logLevel,
                           const std::string& message,
                           const std::string& file,
                           const unsigned int line,
                           const std::string& func,
                           const std::string& rootDir);

    static void logRelog(LogLevel logLevel,
                         const std::istream& stream,
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

/*@{*/
/// Generic logging macro
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


/// Logging errors
#define LOGE(MESSAGE) LOG(ERROR, MESSAGE)

/// Logging warnings
#define LOGW(MESSAGE) LOG(WARN, MESSAGE)

/// Logging information
#define LOGI(MESSAGE) LOG(INFO, MESSAGE)

#if !defined(NDEBUG)
/// Logging debug information
#define LOGD(MESSAGE) LOG(DEBUG, MESSAGE)

/// Logging helper information (for debugging purposes)
#define LOGH(MESSAGE) LOG(HELP, MESSAGE)

/// Logging tracing information
#define LOGT(MESSAGE) LOG(TRACE, MESSAGE)

#define RELOG(ISTREAM)                                                     \
    do {                                                                   \
        if (logger::Logger::getLogLevel() <= logger::LogLevel::DEBUG) {    \
            logger::Logger::logRelog(logger::LogLevel::DEBUG,              \
                                     ISTREAM,                              \
                                     __FILE__,                             \
                                     __LINE__,                             \
                                     __func__,                             \
                                     PROJECT_SOURCE_DIR);                  \
        }                                                                  \
    } while (0)

#else
#define LOGD(MESSAGE) do {} while (0)
#define LOGH(MESSAGE) do {} while (0)
#define LOGT(MESSAGE) do {} while (0)
#define RELOG(ISTREAM) do {} while (0)
#endif

#endif // LOGGER_LOGGER_HPP

/*@}*/
