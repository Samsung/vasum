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
 * @brief   Unit tests of the log utility
 */

#include "config.hpp"

#include "ut.hpp"
#include "logger/logger.hpp"
#include "logger/logger-scope.hpp"
#include "logger/formatter.hpp"
#include "logger/backend.hpp"
#include "logger/backend-stderr.hpp"

#include <stdexcept>

BOOST_AUTO_TEST_SUITE(LoggerSuite)

using namespace logger;

namespace {

class StubbedBackend : public LogBackend {
public:
    StubbedBackend(std::ostringstream& s) : mLogStream(s) {}

    // stubbed function
    void log(LogLevel logLevel,
             const std::string& file,
             const unsigned int& line,
             const std::string& func,
             const std::string& message) override
    {
        mLogStream << '[' + toString(logLevel) + ']' + ' '
                   << file + ':' + std::to_string(line) + ' ' + func + ':'
                   << message << std::endl;
    }

private:
    std::ostringstream& mLogStream;
};

class TestLog {
public:
    TestLog(LogLevel level)
    {
        Logger::setLogLevel(level);
        Logger::setLogBackend(new StubbedBackend(mLogStream));
    }

    ~TestLog()
    {
        Logger::setLogLevel(LogLevel::TRACE);
        Logger::setLogBackend(new StderrBackend());
    }

    // helpers
    bool logContains(const std::string& expression) const
    {
        std::string s = mLogStream.str();
        if (s.find(expression) != std::string::npos) {
            return true;
        }
        return false;
    }
private:
    std::ostringstream mLogStream;
};

void exampleTestLogs(void)
{
    LOGE("test log error " << "1");
    LOGW("test log warn "  << "2");
    LOGI("test log info "  << "3");
    LOGD("test log debug " << "4");
    LOGT("test log trace " << "5");
}

} // namespace

BOOST_AUTO_TEST_CASE(LogLevelSetAndGet)
{
    Logger::setLogLevel(LogLevel::TRACE);
    BOOST_CHECK(LogLevel::TRACE == Logger::getLogLevel());

    Logger::setLogLevel(LogLevel::DEBUG);
    BOOST_CHECK(LogLevel::DEBUG == Logger::getLogLevel());

    Logger::setLogLevel(LogLevel::INFO);
    BOOST_CHECK(LogLevel::INFO == Logger::getLogLevel());

    Logger::setLogLevel(LogLevel::WARN);
    BOOST_CHECK(LogLevel::WARN == Logger::getLogLevel());

    Logger::setLogLevel(LogLevel::ERROR);
    BOOST_CHECK(LogLevel::ERROR == Logger::getLogLevel());
}

BOOST_AUTO_TEST_CASE(StringLogLevelSetAndGet)
{
    Logger::setLogLevel("TRACE");
    BOOST_CHECK(LogLevel::TRACE == Logger::getLogLevel());

    Logger::setLogLevel("traCE");
    BOOST_CHECK(LogLevel::TRACE == Logger::getLogLevel());

    Logger::setLogLevel("DEBUG");
    BOOST_CHECK(LogLevel::DEBUG == Logger::getLogLevel());

    Logger::setLogLevel("INFO");
    BOOST_CHECK(LogLevel::INFO == Logger::getLogLevel());

    Logger::setLogLevel("WARN");
    BOOST_CHECK(LogLevel::WARN == Logger::getLogLevel());

    Logger::setLogLevel("ERROR");
    BOOST_CHECK(LogLevel::ERROR == Logger::getLogLevel());

    BOOST_REQUIRE_EXCEPTION(Logger::setLogLevel("UNKNOWN"),
                            std::runtime_error,
                            WhatEquals("Invalid LogLevel to parse"));
}

BOOST_AUTO_TEST_CASE(LogsLevelError)
{
    TestLog tf(LogLevel::ERROR);
    exampleTestLogs();

    BOOST_CHECK(tf.logContains("[ERROR]") == true);
    BOOST_CHECK(tf.logContains("[WARN]")  == false);
    BOOST_CHECK(tf.logContains("[INFO]")  == false);
    BOOST_CHECK(tf.logContains("[DEBUG]") == false);
    BOOST_CHECK(tf.logContains("[TRACE]") == false);
}

BOOST_AUTO_TEST_CASE(LogsLevelWarn)
{
    TestLog tf(LogLevel::WARN);
    exampleTestLogs();

    BOOST_CHECK(tf.logContains("[ERROR]") == true);
    BOOST_CHECK(tf.logContains("[WARN]")  == true);
    BOOST_CHECK(tf.logContains("[INFO]")  == false);
    BOOST_CHECK(tf.logContains("[DEBUG]") == false);
    BOOST_CHECK(tf.logContains("[TRACE]") == false);
}

BOOST_AUTO_TEST_CASE(LogsLevelInfo)
{
    TestLog tf(LogLevel::INFO);
    exampleTestLogs();

    BOOST_CHECK(tf.logContains("[ERROR]") == true);
    BOOST_CHECK(tf.logContains("[WARN]")  == true);
    BOOST_CHECK(tf.logContains("[INFO]")  == true);
    BOOST_CHECK(tf.logContains("[DEBUG]") == false);
    BOOST_CHECK(tf.logContains("[TRACE]") == false);
}

#if !defined(NDEBUG)
BOOST_AUTO_TEST_CASE(LogsLevelDebug)
{
    TestLog tf(LogLevel::DEBUG);
    exampleTestLogs();

    BOOST_CHECK(tf.logContains("[ERROR]") == true);
    BOOST_CHECK(tf.logContains("[WARN]")  == true);
    BOOST_CHECK(tf.logContains("[INFO]")  == true);
    BOOST_CHECK(tf.logContains("[DEBUG]") == true);
    BOOST_CHECK(tf.logContains("[TRACE]") == false);
}

BOOST_AUTO_TEST_CASE(LogsLevelTrace)
{
    TestLog tf(LogLevel::TRACE);
    exampleTestLogs();

    BOOST_CHECK(tf.logContains("[ERROR]") == true);
    BOOST_CHECK(tf.logContains("[WARN]")  == true);
    BOOST_CHECK(tf.logContains("[INFO]")  == true);
    BOOST_CHECK(tf.logContains("[DEBUG]") == true);
    BOOST_CHECK(tf.logContains("[TRACE]") == true);
}
#endif

BOOST_AUTO_TEST_CASE(LoggerScope)
{
    LOGS("Main function scope");

    {
        LOGS("Scope inside function");
        LOGD("Some additional information in-between scoped logs");
        {
            LOGS("Additional scope with " << "stringstream" << ' ' << "test" << 3 << ' ' << 3.42);
            LOGD("More additional information in-between scoped logs");
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()

