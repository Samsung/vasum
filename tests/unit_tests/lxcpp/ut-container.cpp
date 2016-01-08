/*
 *  Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Jan Olszak (j.olszak@samsung.com)
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
 * @brief   Unit tests of lxcpp Container class
 */

#include "config.hpp"
#include "ut.hpp"

#include "lxcpp/lxcpp.hpp"
#include "lxcpp/exception.hpp"
#include "lxcpp/filesystem.hpp"
#include "lxcpp/smack.hpp"

#include "utils/scoped-dir.hpp"
#include "utils/fs.hpp"
#include "utils/latch.hpp"
#include "utils/spin-wait-for.hpp"

#include <memory>

namespace {

const std::string SIMPLE_INIT         = "/simple_init";
const std::string TEST_DIR            = "/tmp/ut-zones";
const std::string ROOT_DIR            = TEST_DIR + "/root";
const std::string NON_EXISTENT_BINARY = ROOT_DIR + "/nonexistantpath/bash";
const std::string WORK_DIR            = TEST_DIR + "/work";
const std::string LOGGER_FILE         = TEST_DIR + "/loggerFile.txt";

const std::string TESTS_CMD_ROOT            = VSM_TEST_CONFIG_INSTALL_DIR "/utils/";
const std::string TEST_CMD_RANDOM           = "random.sh";
const std::string TEST_CMD_RANDOM_PRODUCT   = "random_product.txt";

const std::vector<std::string> COMMAND = {SIMPLE_INIT};

const int TIMEOUT = 5000; // ms


struct Fixture {
    utils::ScopedDir mTestDir;
    utils::ScopedDir mRoot;
    utils::ScopedDir mRootDev;
    utils::ScopedDir mRootProc;
    utils::ScopedDir mRootSys;
    utils::ScopedDir mWork;

    Fixture()
        :mTestDir(TEST_DIR),
         mRoot(ROOT_DIR),
         mWork(WORK_DIR)
    {
        BOOST_REQUIRE(utils::copyFile(SIMPLE_INIT_PATH, ROOT_DIR + SIMPLE_INIT));
    }

    ~Fixture()
    {
        //grab log file
        std::string log = LOGGER_FILE;
        if (utils::exists(log)) {
            RELOG(std::ifstream(log));
            utils::removeFile(log);
        }
    }
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
    auto c = std::unique_ptr<Container>(createContainer("SetInit", ROOT_DIR, WORK_DIR));

    BOOST_CHECK_THROW(c->setInit({""}), ConfigureException);
    BOOST_CHECK_THROW(c->setInit({}), ConfigureException);
    BOOST_CHECK_THROW(c->setInit({NON_EXISTENT_BINARY}), ConfigureException);

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
    auto c = std::unique_ptr<Container>(createContainer("StartStop", ROOT_DIR, WORK_DIR));
    BOOST_CHECK_NO_THROW(c->setInit(COMMAND));
    BOOST_CHECK_NO_THROW(c->setLogger(logger::LogType::LOG_PERSISTENT_FILE,
                                      logger::LogLevel::TRACE,
                                      LOGGER_FILE));

    BOOST_CHECK_NO_THROW(c->start(TIMEOUT));
    BOOST_CHECK_NO_THROW(c->stop(TIMEOUT));
    BOOST_REQUIRE(utils::spinWaitFor(TIMEOUT, [&] {return c->getState() == Container::State::STOPPED;}));
}

BOOST_AUTO_TEST_CASE(ConnectRunning)
{
    {
        auto c = std::unique_ptr<Container>(createContainer("ConnectRunning", ROOT_DIR, WORK_DIR));
        BOOST_CHECK_NO_THROW(c->setInit(COMMAND));
        BOOST_CHECK_NO_THROW(c->setLogger(logger::LogType::LOG_PERSISTENT_FILE,
                                          logger::LogLevel::DEBUG,
                                          LOGGER_FILE));

        BOOST_CHECK_NO_THROW(c->start());
        BOOST_REQUIRE(utils::spinWaitFor(TIMEOUT, [&] {return c->getState() == Container::State::RUNNING;}));

        // Remove Container class, but don't stop the container
    }
    // Connect to a running container
    auto c = std::unique_ptr<Container>(createContainer("ConnectRunning", ROOT_DIR, WORK_DIR));
    BOOST_CHECK_NO_THROW(c->connect());

    BOOST_CHECK_NO_THROW(c->stop(TIMEOUT));
    BOOST_REQUIRE(utils::spinWaitFor(TIMEOUT, [&] {return c->getState() == Container::State::STOPPED;}));
}

BOOST_AUTO_TEST_CASE(StartCallback)
{
    auto c = std::unique_ptr<Container>(createContainer("StartCallback", ROOT_DIR, WORK_DIR));
    BOOST_CHECK_NO_THROW(c->setInit(COMMAND));
    BOOST_CHECK_NO_THROW(c->setLogger(logger::LogType::LOG_PERSISTENT_FILE,
                                      logger::LogLevel::DEBUG,
                                      LOGGER_FILE));

    utils::Latch latch;
    auto call = [&latch]() {
        latch.set();
    };
    c->setStartedCallback(call);
    BOOST_CHECK_NO_THROW(c->start(TIMEOUT));

    BOOST_REQUIRE(latch.wait(TIMEOUT));

    BOOST_CHECK_NO_THROW(c->stop(TIMEOUT));
    BOOST_REQUIRE(c->getState() != Container::State::RUNNING);

    auto pred2 = [&] {return c->getState() == Container::State::STOPPED;};
    BOOST_REQUIRE(utils::spinWaitFor(TIMEOUT, pred2));
}

BOOST_AUTO_TEST_CASE(StopCallback)
{
    auto c = std::unique_ptr<Container>(createContainer("StopCallback", ROOT_DIR, WORK_DIR));
    BOOST_CHECK_NO_THROW(c->setInit(COMMAND));
    BOOST_CHECK_NO_THROW(c->setLogger(logger::LogType::LOG_PERSISTENT_FILE,
                                      logger::LogLevel::DEBUG,
                                      LOGGER_FILE));

    BOOST_CHECK_NO_THROW(c->start(TIMEOUT));

    utils::Latch latch;
    auto call = [&latch]() {
        latch.set();
    };
    c->setStoppedCallback(call);

    BOOST_CHECK_NO_THROW(c->stop(TIMEOUT));
    BOOST_REQUIRE(latch.wait(TIMEOUT));
    BOOST_REQUIRE(c->getState() == Container::State::STOPPED);
}

BOOST_AUTO_TEST_CASE(ConnectCallback)
{
    {
        auto c = std::unique_ptr<Container>(createContainer("ConnectCallback", ROOT_DIR, WORK_DIR));
        BOOST_CHECK_NO_THROW(c->setInit(COMMAND));
        BOOST_CHECK_NO_THROW(c->setLogger(logger::LogType::LOG_PERSISTENT_FILE,
                                          logger::LogLevel::TRACE,
                                          LOGGER_FILE));

        BOOST_CHECK_NO_THROW(c->start(TIMEOUT));
        BOOST_REQUIRE(utils::spinWaitFor(TIMEOUT, [&] {return c->getState() == Container::State::RUNNING;}));

        // Remove Container class, but don't stop the container
    }
    // Connect to a running container
    auto c = std::unique_ptr<Container>(createContainer("ConnectCallback", ROOT_DIR, WORK_DIR));

    utils::Latch latch;
    auto call = [&latch]() {
        latch.set();
    };
    c->setConnectedCallback(call);

    BOOST_CHECK_NO_THROW(c->connect());
    BOOST_REQUIRE(latch.wait(TIMEOUT));
    BOOST_REQUIRE(c->getState() == Container::State::RUNNING);

    BOOST_CHECK_NO_THROW(c->stop(TIMEOUT));
    BOOST_REQUIRE(utils::spinWaitFor(TIMEOUT, [&] {return c->getState() == Container::State::STOPPED;}));
}

BOOST_AUTO_TEST_CASE(UIDGIDGoodMapping)
{
    BOOST_CHECK_NO_THROW(lxcpp::chown(ROOT_DIR, 1000, 1000));

    auto c = std::unique_ptr<Container>(createContainer("UIDGIDGoodMapping", ROOT_DIR, WORK_DIR));
    BOOST_CHECK_NO_THROW(c->setInit(COMMAND));
    BOOST_CHECK_NO_THROW(c->setLogger(logger::LogType::LOG_PERSISTENT_FILE,
                                      logger::LogLevel::DEBUG,
                                      LOGGER_FILE));

    c->addUIDMap(0, 1000, 1000);
    c->addUIDMap(1000, 0, 999);
    c->addGIDMap(0, 1000, 1000);
    c->addGIDMap(1000, 0, 999);

    BOOST_CHECK_NO_THROW(c->start(TIMEOUT));
    BOOST_CHECK_NO_THROW(c->stop(TIMEOUT));
    BOOST_REQUIRE(utils::spinWaitFor(TIMEOUT, [&] {return c->getState() == Container::State::STOPPED;}));
}

BOOST_AUTO_TEST_CASE(UIDBadMapping)
{
    auto c = std::unique_ptr<Container>(createContainer("UIDBadMapping", ROOT_DIR, WORK_DIR));
    BOOST_CHECK_NO_THROW(c->setInit(COMMAND));
    BOOST_CHECK_NO_THROW(c->setLogger(logger::LogType::LOG_PERSISTENT_FILE,
                                      logger::LogLevel::DEBUG,
                                      LOGGER_FILE));

    // At most 5 mappings allowed
    for(int i = 0; i < 5; ++i) {
        c->addUIDMap(0, 1000, 1);
    }

    BOOST_REQUIRE_THROW(c->addUIDMap(0, 1000, 1),ConfigureException);
}

BOOST_AUTO_TEST_CASE(GIDBadMapping)
{
    auto c = std::unique_ptr<Container>(createContainer("GIDBadMapping", ROOT_DIR, WORK_DIR));
    BOOST_CHECK_NO_THROW(c->setInit(COMMAND));
    BOOST_CHECK_NO_THROW(c->setLogger(logger::LogType::LOG_PERSISTENT_FILE,
                                      logger::LogLevel::DEBUG,
                                      LOGGER_FILE));

    for(int i = 0; i < 5; ++i) {
        c->addGIDMap(0, 1000, 1);
    }

    BOOST_REQUIRE_THROW(c->addGIDMap(0, 1000, 1),ConfigureException);
}

BOOST_AUTO_TEST_CASE(SmackMapping)
{
    // below test should be executed only when Smack Namespace is available
    if (!isSmackNamespaceActive()) {
        return;
    }

    auto c = std::unique_ptr<Container>(createContainer("SmackMapping", ROOT_DIR, WORK_DIR));
    BOOST_CHECK_NO_THROW(c->setInit(COMMAND));
    BOOST_CHECK_NO_THROW(c->setLogger(logger::LogType::LOG_PERSISTENT_FILE,
                                      logger::LogLevel::DEBUG,
                                      LOGGER_FILE));

    c->addSmackLabelMap("original", "mapped");
    c->addSmackLabelMap("second", "secondMapped");

    BOOST_CHECK_NO_THROW(c->start(TIMEOUT));
    BOOST_CHECK_NO_THROW(c->stop(TIMEOUT));
    BOOST_REQUIRE(utils::spinWaitFor(TIMEOUT, [&] {return c->getState() == Container::State::STOPPED;}));
}

BOOST_AUTO_TEST_SUITE_END()
