/*
 *  Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
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
 * @brief   Unit tests of Inotify
 */

#include "config.hpp"

#include "ut.hpp"

#include "utils/inotify.hpp"
#include "utils/fs.hpp"
#include "utils/scoped-dir.hpp"
#include "utils/value-latch.hpp"

#include "logger/logger.hpp"

#include "cargo-ipc/epoll/event-poll.hpp"
#include "cargo-ipc/epoll/thread-dispatcher.hpp"

#include <boost/filesystem.hpp>

using namespace utils;
namespace fs = boost::filesystem;

namespace {

const std::string TEST_DIR  = "/tmp/ut-inotify/";
const std::string DIR_NAME  = "dir";
const std::string FILE_NAME = "file.txt";

const std::string DIR_PATH  = TEST_DIR + DIR_NAME;
const std::string FILE_PATH = TEST_DIR + FILE_NAME;


struct Fixture {
    utils::ScopedDir mTestDir;

    Fixture()
        :mTestDir(TEST_DIR)
    {}
};

} // namespace


BOOST_FIXTURE_TEST_SUITE(InotifySuite, Fixture)

BOOST_AUTO_TEST_CASE(ConstructorDesctructor)
{
    cargo::ipc::epoll::EventPoll poll;
    Inotify i(poll);
}

BOOST_AUTO_TEST_CASE(CreateDeleteFileHandler)
{
    cargo::ipc::epoll::ThreadDispatcher dispatcher;
    Inotify i(dispatcher.getPoll());

    // Callback on creation
    ValueLatch<std::string> createResult;
    i.setHandler(TEST_DIR, IN_CREATE, [&](const std::string& name, uint32_t) {
        createResult.set(name);
    });
    utils::createFile(FILE_PATH, O_WRONLY | O_CREAT, 0666);
    BOOST_REQUIRE_EQUAL(createResult.get(), FILE_NAME);

    // Redefine the callback for delete
    ValueLatch<std::string> deleteResult;
    i.setHandler(TEST_DIR, IN_DELETE, [&](const std::string& name, uint32_t) {
        deleteResult.set(name);
    });
    fs::remove(FILE_PATH);
    BOOST_REQUIRE_EQUAL(deleteResult.get(), FILE_NAME);
}

BOOST_AUTO_TEST_CASE(CreateDeleteDirHandler)
{
    cargo::ipc::epoll::ThreadDispatcher dispatcher;
    Inotify i(dispatcher.getPoll());

    // Callback on creation
    ValueLatch<std::string> createResult;
    i.setHandler(TEST_DIR, IN_CREATE, [&](const std::string& name, uint32_t) {
        createResult.set(name);
    });
    utils::createEmptyDir(DIR_PATH);
    BOOST_REQUIRE_EQUAL(createResult.get(), DIR_NAME);


    // Redefine the callback for delete
    ValueLatch<std::string> deleteResult;
    i.setHandler(TEST_DIR, IN_DELETE, [&](const std::string& name, uint32_t) {
        deleteResult.set(name);
    });
    fs::remove_all(DIR_PATH);
    BOOST_REQUIRE_EQUAL(deleteResult.get(), DIR_NAME);
}

BOOST_AUTO_TEST_CASE(NoFalseEventHandler)
{
    cargo::ipc::epoll::ThreadDispatcher dispatcher;
    Inotify i(dispatcher.getPoll());

    utils::createFile(FILE_PATH, O_WRONLY | O_CREAT, 0666);

    // Callback on creation shouldn't be called
    ValueLatch<std::string> createResult;
    i.setHandler(TEST_DIR, IN_CREATE, [&](const std::string& name, uint32_t) {
        createResult.set(name);
    });
    fs::remove(FILE_PATH);
    BOOST_REQUIRE_THROW(createResult.get(10), UtilsException);
}

BOOST_AUTO_TEST_CASE(RemoveHandler)
{
    cargo::ipc::epoll::ThreadDispatcher dispatcher;
    Inotify i(dispatcher.getPoll());

    // Callback on creation
    ValueLatch<std::string> createResult;
    i.setHandler(TEST_DIR, IN_CREATE, [&](const std::string& name, uint32_t) {
        createResult.set(name);
    });
    i.removeHandler(TEST_DIR);
    utils::createFile(FILE_PATH, O_WRONLY | O_CREAT, 0666);
    fs::remove(FILE_PATH);
    BOOST_REQUIRE_THROW(createResult.get(10), UtilsException);
}

BOOST_AUTO_TEST_SUITE_END()
