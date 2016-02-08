/*
 *  Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Maciej Karpiuk (m.karpiuk2@samsung.com)
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
 * @author  Maciej Karpiuk (m.karpiuk2@samsung.com)
 * @brief   Unit tests of lxcpp attach to container method
 */

#include "config.hpp"
#include "ut.hpp"

#include "lxcpp/lxcpp.hpp"
#include "lxcpp/exception.hpp"

#include "utils/scoped-dir.hpp"
#include "utils/signal.hpp"
#include "utils/fs.hpp"
#include "utils/spin-wait-for.hpp"

#include <signal.h>
#include <memory>

namespace {

const std::string SIMPLE_INIT               = "/simple_init";
const std::string SIMPLE_RAND               = "/simple_rand";
const std::string TEST_DIR                  = "/tmp/ut-attach";
const std::string ROOT_DIR                  = TEST_DIR + "/root";
const std::string WORK_DIR                  = TEST_DIR + "/work";
const std::string LOGGER_FILE               = TEST_DIR + "/loggerFile";

const std::string TESTS_CMD_ROOT            = VSM_TEST_CONFIG_INSTALL_DIR "/utils/";
const std::string TEST_CMD_RANDOM_PRODUCT   = "random_product.txt";

const int TIMEOUT = 3000; //ms

const std::vector<std::string> COMMAND = {SIMPLE_INIT};

struct Fixture {
    utils::ScopedDir mTestDir;
    utils::ScopedDir mRootDir;
    utils::ScopedDir mWork;

    Fixture()
        :mTestDir(TEST_DIR),
         mRootDir(ROOT_DIR),
         mWork(WORK_DIR)
    {
        BOOST_REQUIRE_NO_THROW(utils::copyFile(SIMPLE_INIT_PATH, ROOT_DIR + SIMPLE_INIT));
        BOOST_REQUIRE_NO_THROW(utils::copyFile(SIMPLE_RAND_PATH, ROOT_DIR + SIMPLE_RAND));

        container = std::unique_ptr<lxcpp::Container>(lxcpp::createContainer("Attach", ROOT_DIR, WORK_DIR));
        BOOST_CHECK_NO_THROW(container->setInit(COMMAND));
        BOOST_CHECK_NO_THROW(container->setLogger(logger::LogType::LOG_PERSISTENT_FILE,
                                                  logger::LogLevel::DEBUG,
                                                  LOGGER_FILE));

        utils::signalBlock(SIGCHLD);
    }

    ~Fixture()
    {
        //grab log file
        std::string log = LOGGER_FILE;
        if (utils::exists(log)) {
            RELOG(std::ifstream(log));
            utils::remove(log);
        }
        utils::signalUnblock(SIGCHLD);
    }

    int attach(const std::vector<std::string>& args,
               const std::string& cwdInContainer)
    {
        int ret;
        BOOST_REQUIRE_NO_THROW(ret = container->attach(
                                                 args,  // argv
                                                 0, 0,  // uid, gid
                                                 std::string(), // ttyPath
                                                 {},    // supplementaryGids
                                                 0,     // capsToKeep
                                                 cwdInContainer,
                                                 {},    // envToKeep
                                                 {}     // envToSet
                                                 ));
        return ret;
    }

    std::unique_ptr<lxcpp::Container> container;
};

} // namespace

BOOST_FIXTURE_TEST_SUITE(LxcppContainerAttachSuite, Fixture)

using namespace lxcpp;

BOOST_AUTO_TEST_CASE(Attach)
{
    BOOST_REQUIRE_NO_THROW(container->start());
    BOOST_REQUIRE(utils::spinWaitFor(TIMEOUT, [&] {return container->getState() == Container::State::RUNNING;}));
    attach({SIMPLE_RAND, "16", TEST_CMD_RANDOM_PRODUCT}, "/");
    BOOST_REQUIRE_NO_THROW(container->stop());
    BOOST_REQUIRE(utils::spinWaitFor(TIMEOUT, [&] {return container->getState() == Container::State::STOPPED;}));

    std::string random;
    BOOST_REQUIRE_NO_THROW(random = utils::readFileContent(ROOT_DIR + "/" + TEST_CMD_RANDOM_PRODUCT));
    BOOST_REQUIRE_GT(random.size(), 0);
}

BOOST_AUTO_TEST_CASE(AttachGetResponseCode)
{
    BOOST_REQUIRE_NO_THROW(container->start());
    BOOST_REQUIRE(utils::spinWaitFor(TIMEOUT, [&] {return container->getState() == Container::State::RUNNING;}));
    BOOST_REQUIRE(attach({SIMPLE_RAND}, "/") != 0);
    BOOST_REQUIRE_NO_THROW(container->stop());
    BOOST_REQUIRE(utils::spinWaitFor(TIMEOUT, [&] {return container->getState() == Container::State::STOPPED;}));
}

BOOST_AUTO_TEST_SUITE_END()
