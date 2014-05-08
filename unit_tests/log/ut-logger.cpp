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

#include "ut.hpp"
#include "log/logger.hpp"
#include "log/formatter.hpp"
#include "log/backend.hpp"
#include "log/backend-stderr.hpp"


BOOST_AUTO_TEST_SUITE(LogSuite)

using namespace security_containers::log;

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
        mLogStream << '[' + LogFormatter::toString(logLevel) + ']' + ' '
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

BOOST_AUTO_TEST_CASE(LogLevelSetandGet)
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

BOOST_AUTO_TEST_CASE(TestLogsError)
{
    TestLog tf(LogLevel::ERROR);
    exampleTestLogs();

    BOOST_CHECK(tf.logContains("[ERROR]") == true);
    BOOST_CHECK(tf.logContains("[WARN]")  == false);
    BOOST_CHECK(tf.logContains("[INFO]")  == false);
    BOOST_CHECK(tf.logContains("[DEBUG]") == false);
    BOOST_CHECK(tf.logContains("[TRACE]") == false);
}

BOOST_AUTO_TEST_CASE(TestLogsWarn)
{
    TestLog tf(LogLevel::WARN);
    exampleTestLogs();

    BOOST_CHECK(tf.logContains("[ERROR]") == true);
    BOOST_CHECK(tf.logContains("[WARN]")  == true);
    BOOST_CHECK(tf.logContains("[INFO]")  == false);
    BOOST_CHECK(tf.logContains("[DEBUG]") == false);
    BOOST_CHECK(tf.logContains("[TRACE]") == false);
}

BOOST_AUTO_TEST_CASE(TestLogsInfo)
{
    TestLog tf(LogLevel::INFO);
    exampleTestLogs();

    BOOST_CHECK(tf.logContains("[ERROR]") == true);
    BOOST_CHECK(tf.logContains("[WARN]")  == true);
    BOOST_CHECK(tf.logContains("[INFO]")  == true);
    BOOST_CHECK(tf.logContains("[DEBUG]") == false);
    BOOST_CHECK(tf.logContains("[TRACE]") == false);
}

BOOST_AUTO_TEST_CASE(TestLogsDebug)
{
    TestLog tf(LogLevel::DEBUG);
    exampleTestLogs();

    BOOST_CHECK(tf.logContains("[ERROR]") == true);
    BOOST_CHECK(tf.logContains("[WARN]")  == true);
    BOOST_CHECK(tf.logContains("[INFO]")  == true);
    BOOST_CHECK(tf.logContains("[DEBUG]") == true);
    BOOST_CHECK(tf.logContains("[TRACE]") == false);
}

BOOST_AUTO_TEST_CASE(TestLogsTrace)
{
    TestLog tf(LogLevel::TRACE);
    exampleTestLogs();

    BOOST_CHECK(tf.logContains("[ERROR]") == true);
    BOOST_CHECK(tf.logContains("[WARN]")  == true);
    BOOST_CHECK(tf.logContains("[INFO]")  == true);
    BOOST_CHECK(tf.logContains("[DEBUG]") == true);
    BOOST_CHECK(tf.logContains("[TRACE]") == true);
}


BOOST_AUTO_TEST_SUITE_END()

