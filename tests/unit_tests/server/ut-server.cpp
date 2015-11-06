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
 * @brief   Unit tests of the Server class
 */

#include "config.hpp"
#include "ut.hpp"

#include "server.hpp"
#include "zones-manager.hpp"
#include "exception.hpp"
#include "cargo/exception.hpp"
#include "utils/glib-loop.hpp"
#include "utils/scoped-dir.hpp"
#include "logger/logger.hpp"
#include "ipc/epoll/thread-dispatcher.hpp"

#include <string>
#include <future>

using namespace utils;
using namespace vasum;
using namespace cargo;


namespace {

const std::string CONFIG_DIR = VSM_TEST_CONFIG_INSTALL_DIR;
const std::string TEST_CONFIG_PATH = CONFIG_DIR + "/test-daemon.conf";
const std::string MISSING_CONFIG_PATH = CONFIG_DIR + "/missing-daemon.conf";
const std::string TEMPLATE_NAME = "default";

const std::string ZONES_PATH = "/tmp/ut-zones"; // the same as in daemon.conf
const bool AS_ROOT = true;

struct Fixture {
    utils::ScopedDir mZonesPathGuard;

    Fixture()
        : mZonesPathGuard(ZONES_PATH)
    {
        LOGI("------------ ServerSuite fixture -----------");
        prepare();
        LOGI("------------ setup complete -----------");
    }

    static void prepare()
    {
        ScopedGlibLoop loop;
        ipc::epoll::ThreadDispatcher mDispatcher;
        ZonesManager manager(mDispatcher.getPoll(), TEST_CONFIG_PATH);
        manager.start();
        manager.createZone("zone1", TEMPLATE_NAME);
        manager.createZone("zone2", TEMPLATE_NAME);
        manager.restoreAll();
        manager.stop(true);
    }
};
} // namespace

BOOST_FIXTURE_TEST_SUITE(ServerSuite, Fixture)

BOOST_AUTO_TEST_CASE(ConstructorDestructor)
{
    std::unique_ptr<Server> s;
    s.reset(new Server(TEST_CONFIG_PATH));
    s.reset();
}

BOOST_AUTO_TEST_CASE(MissingConfig)
{
    BOOST_REQUIRE_EXCEPTION(Server(MISSING_CONFIG_PATH).run(AS_ROOT),
                            CargoException,
                            WhatEquals("Could not load " + MISSING_CONFIG_PATH));
}

BOOST_AUTO_TEST_CASE(Terminate)
{
    Server s(TEST_CONFIG_PATH);
    s.terminate();
}

BOOST_AUTO_TEST_CASE(TerminateRun)
{
    Server s(TEST_CONFIG_PATH);
    s.terminate();
    s.run(AS_ROOT);
}

BOOST_AUTO_TEST_CASE(RunTerminate)
{
    Server s(TEST_CONFIG_PATH);
    std::future<void> runFuture = std::async(std::launch::async, [&] {
        // give a chance to run
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        s.terminate();
    });

    s.run(AS_ROOT);
    runFuture.wait();

    // a potential exception from std::async thread will be delegated to this thread
    runFuture.get();
}


BOOST_AUTO_TEST_SUITE_END()
