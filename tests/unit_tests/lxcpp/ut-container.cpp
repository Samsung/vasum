/*
 *  Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Jan Olszak(j.olszak@samsung.com)
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
 * @author  Jan Olszak(j.olszak@samsung.com)
 * @brief   Unit tests of lxcpp Container class
 */

#include "config.hpp"
#include "ut.hpp"

#include "lxcpp/lxcpp.hpp"
#include "lxcpp/exception.hpp"

#include "utils/scoped-dir.hpp"
#include "utils/fs.hpp"

#include <memory>

namespace {

const std::string TEST_DIR            = "/tmp/ut-zones";
const std::string ROOT_DIR            = TEST_DIR + "/root";
const std::string WORK_DIR            = TEST_DIR + "/work";
const std::string NON_EXISTANT_BINARY = TEST_DIR + "/nonexistantpath/bash";
const std::string LOGGER_FILE         = TEST_DIR + "/loggerFile.txt";

const std::string TESTS_CMD_ROOT            = VSM_TEST_CONFIG_INSTALL_DIR "/utils/";
const std::string TEST_CMD_RANDOM           = "random.sh";
const std::string TEST_CMD_RANDOM_PRODUCT   = "random_product.txt";

const std::vector<std::string> COMMAND = {"/bin/bash",
                                          "-c", "trap exit SIGTERM; while true; do sleep 0.1; done"
                                         };

struct Fixture {
    utils::ScopedDir mTestDir;
    utils::ScopedDir mRoot;
    utils::ScopedDir mWork;

    Fixture()
        :mTestDir(TEST_DIR),
         mRoot(ROOT_DIR),
         mWork(WORK_DIR)
    {}
    ~Fixture() {}
};

} // namespace

BOOST_FIXTURE_TEST_SUITE(LxcppContainerSuite, Fixture)

using namespace lxcpp;

BOOST_AUTO_TEST_CASE(ConstructorDestructor)
{
    auto c = createContainer("ConstructorDestructor", ROOT_DIR, WORK_DIR);
    delete c;
}

BOOST_AUTO_TEST_CASE(SetInit)
{
    auto c = std::unique_ptr<Container>(createContainer("SetInit", "/", WORK_DIR));

    BOOST_CHECK_THROW(c->setInit({""}), ConfigureException);
    BOOST_CHECK_THROW(c->setInit({}), ConfigureException);
    BOOST_CHECK_THROW(c->setInit({NON_EXISTANT_BINARY}), ConfigureException);

    BOOST_CHECK_NO_THROW(c->setInit(COMMAND));
}

BOOST_AUTO_TEST_CASE(SetLogger)
{
    auto c = std::unique_ptr<Container>(createContainer("SetLogger", ROOT_DIR, WORK_DIR));

    BOOST_CHECK_NO_THROW(c->setLogger(logger::LogType::LOG_NULL,
                                      logger::LogLevel::DEBUG));
    BOOST_CHECK_NO_THROW(c->setLogger(logger::LogType::LOG_JOURNALD,
                                      logger::LogLevel::DEBUG));
    BOOST_CHECK_NO_THROW(c->setLogger(logger::LogType::LOG_SYSLOG,
                                      logger::LogLevel::DEBUG));
    BOOST_CHECK_NO_THROW(c->setLogger(logger::LogType::LOG_STDERR,
                                      logger::LogLevel::DEBUG));

    BOOST_CHECK_NO_THROW(c->setLogger(logger::LogType::LOG_FILE,
                                      logger::LogLevel::DEBUG,
                                      LOGGER_FILE));
    BOOST_CHECK_NO_THROW(c->setLogger(logger::LogType::LOG_PERSISTENT_FILE,
                                      logger::LogLevel::DEBUG,
                                      LOGGER_FILE));

    BOOST_CHECK_THROW(c->setLogger(logger::LogType::LOG_FILE,
                                   logger::LogLevel::DEBUG,
                                   ""),
                      BadArgument);

    BOOST_CHECK_THROW(c->setLogger(logger::LogType::LOG_PERSISTENT_FILE,
                                   logger::LogLevel::DEBUG,
                                   ""),
                      BadArgument);
}

BOOST_AUTO_TEST_CASE(StartStop)
{
    auto c = std::unique_ptr<Container>(createContainer("StartStop", "/", WORK_DIR));
    BOOST_CHECK_NO_THROW(c->setInit(COMMAND));
    BOOST_CHECK_NO_THROW(c->setLogger(logger::LogType::LOG_PERSISTENT_FILE,
                                      logger::LogLevel::DEBUG,
                                      LOGGER_FILE));
    c->start();
    // FIXME: Remove the sleep
    sleep(3);
    BOOST_CHECK_NO_THROW(c->stop());
    sleep(2);
}

// BOOST_AUTO_TEST_CASE(Attach)
// {
//     auto c = std::unique_ptr<Container>(createContainer("Attach", "/", WORK_DIR));
//     BOOST_CHECK_NO_THROW(c->setInit(COMMAND));
//     BOOST_CHECK_NO_THROW(c->setLogger(logger::LogType::LOG_PERSISTENT_FILE,
//                                       logger::LogLevel::DEBUG,
//                                       LOGGER_FILE));
//     BOOST_CHECK_NO_THROW(c->start());
//     BOOST_CHECK_NO_THROW(c->attach({TESTS_CMD_ROOT + TEST_CMD_RANDOM, TEST_CMD_RANDOM_PRODUCT}, TEST_DIR));
//     BOOST_CHECK_NO_THROW(c->stop());

//     std::string random;
//     BOOST_REQUIRE_NO_THROW(utils::readFileContent(TEST_DIR + "/" + TEST_CMD_RANDOM_PRODUCT, random));
//     BOOST_ASSERT(random.size() > 0);
// }

BOOST_AUTO_TEST_SUITE_END()
