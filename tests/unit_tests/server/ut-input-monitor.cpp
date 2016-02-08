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
 * @brief   Unit tests of the InputMonitor class
 */

#include "config.hpp"

#include "ut.hpp"

#include "input-monitor.hpp"
#include "input-monitor-config.hpp"
#include "exception.hpp"
#include "zones-manager.hpp"

#include "utils/glib-loop.hpp"
#include "utils/fs.hpp"
#include "utils/latch.hpp"
#include "utils/scoped-dir.hpp"
#include "cargo-ipc/epoll/thread-dispatcher.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include <chrono>
#include <memory>
#include <string>

using namespace vasum;
using namespace utils;

namespace {

const std::string TEST_DIR = "/tmp/ut-input-monitor";
const std::string TEST_INPUT_DEVICE = TEST_DIR + "/input-device";

const int EVENT_TYPE = 1;
const int EVENT_CODE = 139;
const int EVENT_BUTTON_RELEASED = 0;
const int EVENT_BUTTON_PRESSED = 1;

const int EVENT_TIMEOUT = 1000;

const std::string CONFIG_DIR = VSM_TEST_CONFIG_INSTALL_DIR;
const std::string TEST_CONFIG_PATH = CONFIG_DIR + "/test-daemon.conf";
const std::string SIMPLE_TEMPLATE = "console-ipc";
const std::string ZONES_PATH = "/tmp/ut-zones"; // the same as in daemon.conf

struct Fixture {
    utils::ScopedGlibLoop mLoop;
    ScopedDir mTestPathGuard;
    ScopedDir mZonesPathGuard;
    InputConfig inputConfig;
    struct input_event ie;
    cargo::ipc::epoll::ThreadDispatcher dispatcher;

    Fixture()
        : mTestPathGuard(TEST_DIR)
        , mZonesPathGuard(ZONES_PATH)
    {
        inputConfig.numberOfEvents = 2;
        inputConfig.device = TEST_INPUT_DEVICE;
        inputConfig.code = EVENT_CODE;
        inputConfig.enabled = true;
        inputConfig.timeWindowMs = 500;

        // fill simulated events with init values
        ie.time.tv_sec = 946707544;
        ie.time.tv_usec = 0;
        ie.type = EVENT_TYPE;
        ie.code = EVENT_CODE;
        ie.value = EVENT_BUTTON_RELEASED;

        BOOST_CHECK_NO_THROW(utils::mkfifo(TEST_INPUT_DEVICE, S_IWUSR | S_IRUSR));
    }

    cargo::ipc::epoll::EventPoll& getPoll() {
        return dispatcher.getPoll();
    }
};

template<class Predicate>
bool spinWaitFor(int timeoutMs, Predicate pred)
{
    auto until = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    while (!pred()) {
        if (std::chrono::steady_clock::now() >= until) {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return true;
}

} // namespace

BOOST_FIXTURE_TEST_SUITE(InputMonitorSuite, Fixture)

BOOST_AUTO_TEST_CASE(ConfigOK)
{
    ZonesManager cm(getPoll(), TEST_CONFIG_PATH);
    InputMonitor inputMonitor(getPoll(), inputConfig, &cm);
}

BOOST_AUTO_TEST_CASE(ConfigTimeWindowMsTooHigh)
{
    ZonesManager cm(getPoll(), TEST_CONFIG_PATH);
    inputConfig.timeWindowMs = 50000;

    BOOST_REQUIRE_THROW(InputMonitor inputMonitor(getPoll(), inputConfig, &cm),
                        TimeoutException);
}

BOOST_AUTO_TEST_CASE(ConfigDeviceFilePathNotExisting)
{
    ZonesManager cm(getPoll(), TEST_CONFIG_PATH);
    inputConfig.device = TEST_INPUT_DEVICE + "notExisting";

    BOOST_REQUIRE_EXCEPTION(InputMonitor inputMonitor(getPoll(), inputConfig, &cm),
                            InputMonitorException,
                            WhatEquals("Cannot find a device"));
}

namespace {

void sendEvent(Fixture& f, ZonesManager& cm)
{
    InputMonitor inputMonitor(f.getPoll(), f.inputConfig, &cm);
    inputMonitor.start();
    int fd = ::open(TEST_INPUT_DEVICE.c_str(), O_WRONLY);
    BOOST_REQUIRE(fd >= 0);

    for (int i = 0; i < f.inputConfig.numberOfEvents; ++i) {
        // button pressed event
        f.ie.value = EVENT_BUTTON_PRESSED;
        f.ie.time.tv_usec += 5;
        ssize_t ret = ::write(fd, &f.ie, sizeof(struct input_event));
        BOOST_CHECK(ret > 0);

        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        // button released event
        f.ie.value = EVENT_BUTTON_RELEASED;
        f.ie.time.tv_usec += 5;
        ret = ::write(fd, &f.ie, sizeof(struct input_event));
        BOOST_CHECK(ret > 0);
    }

    BOOST_CHECK(::close(fd) >= 0);
}

} // namespace

BOOST_AUTO_TEST_CASE(SingleEvent)
{
    ZonesManager cm(getPoll(), TEST_CONFIG_PATH);
    cm.start();
    cm.createZone("zone1", SIMPLE_TEMPLATE);
    cm.createZone("zone2", SIMPLE_TEMPLATE);
    cm.createZone("zone3", SIMPLE_TEMPLATE);
    cm.restoreAll();
    sendEvent(*this, cm);
    BOOST_CHECK(spinWaitFor(EVENT_TIMEOUT, [&] {
            return cm.getRunningForegroundZoneId() == "zone2";
    }));
}

BOOST_AUTO_TEST_CASE(MultipleEvent)
{
    ZonesManager cm(getPoll(), TEST_CONFIG_PATH);
    cm.start();
    cm.createZone("zone1", SIMPLE_TEMPLATE);
    cm.createZone("zone2", SIMPLE_TEMPLATE);
    cm.createZone("zone3", SIMPLE_TEMPLATE);
    cm.restoreAll();
    for (int i = 1; i < 10; ++i) {
        sendEvent(*this, cm);
        std::string zoneId = "zone" + std::to_string(i % 3 + 1);
        BOOST_CHECK(spinWaitFor(EVENT_TIMEOUT, [&] {
            return cm.getRunningForegroundZoneId() == zoneId;
        }));
    }
}

namespace {

void sendEventWithPauses(Fixture& f, ZonesManager& cm)
{
    InputMonitor inputMonitor(f.getPoll(), f.inputConfig, &cm);
    inputMonitor.start();

    int fd = ::open(TEST_INPUT_DEVICE.c_str(), O_WRONLY);
    BOOST_REQUIRE(fd >= 0);

    for (int i = 0; i < f.inputConfig.numberOfEvents; ++i) {
        // Send first two bytes of the button pressed event
        f.ie.value = EVENT_BUTTON_PRESSED;
        f.ie.time.tv_usec += 5;
        ssize_t ret = ::write(fd, &f.ie, 2);
        BOOST_CHECK(ret > 0);

        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        // Send the remaining part
        ret = ::write(fd, &reinterpret_cast<char*>(&f.ie)[2], sizeof(struct input_event) - 2);
        BOOST_CHECK(ret > 0);

        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        // Send the button released event
        f.ie.value = EVENT_BUTTON_RELEASED;
        f.ie.time.tv_usec += 5;
        ret = ::write(fd, &f.ie, sizeof(struct input_event));
        BOOST_CHECK(ret > 0);
    }
    BOOST_CHECK(::close(fd) >= 0);
}

} // namespace

BOOST_AUTO_TEST_CASE(SingleEventWithPauses)
{
    ZonesManager cm(getPoll(), TEST_CONFIG_PATH);
    cm.start();
    cm.createZone("zone1", SIMPLE_TEMPLATE);
    cm.createZone("zone2", SIMPLE_TEMPLATE);
    cm.createZone("zone3", SIMPLE_TEMPLATE);
    cm.restoreAll();
    sendEventWithPauses(*this, cm);
    BOOST_CHECK(spinWaitFor(EVENT_TIMEOUT, [&] {
            return cm.getRunningForegroundZoneId() == "zone2";
    }));
}

BOOST_AUTO_TEST_CASE(MultipleEventWithPauses)
{
    ZonesManager cm(getPoll(), TEST_CONFIG_PATH);
    cm.start();
    cm.createZone("zone1", SIMPLE_TEMPLATE);
    cm.createZone("zone2", SIMPLE_TEMPLATE);
    cm.createZone("zone3", SIMPLE_TEMPLATE);
    cm.restoreAll();
    for (int i = 1; i < 10; ++i) {
        sendEventWithPauses(*this, cm);
        std::string zoneId = "zone" + std::to_string(i % 3 + 1);
        BOOST_CHECK(spinWaitFor(EVENT_TIMEOUT, [&] {
            return cm.getRunningForegroundZoneId() == zoneId;
        }));
    }
}


BOOST_AUTO_TEST_SUITE_END()
