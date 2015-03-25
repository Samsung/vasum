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

#include "utils/glib-loop.hpp"
#include "utils/latch.hpp"
#include "utils/scoped-dir.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include <chrono>
#include <memory>
#include <string>


using namespace vasum;
using namespace vasum::utils;

namespace {

const std::string TEST_DIR = "/tmp/ut-input-monitor";
const std::string TEST_INPUT_DEVICE = TEST_DIR + "/input-device";

const int EVENT_TYPE = 1;
const int EVENT_CODE = 139;
const int EVENT_BUTTON_RELEASED = 0;
const int EVENT_BUTTON_PRESSED = 1;

const int SINGLE_EVENT_TIMEOUT = 1000;

struct Fixture {
    utils::ScopedGlibLoop mLoop;
    ScopedDir mTestPathGuard;
    InputConfig inputConfig;
    struct input_event ie;

    Fixture()
        : mTestPathGuard(TEST_DIR)
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

        BOOST_CHECK(::mkfifo(TEST_INPUT_DEVICE.c_str(), S_IWUSR | S_IRUSR) >= 0);
    }
};
} // namespace

BOOST_FIXTURE_TEST_SUITE(InputMonitorSuite, Fixture)

BOOST_AUTO_TEST_CASE(ConfigOK)
{
    InputMonitor inputMonitor(inputConfig, InputMonitor::NotifyCallback());
}

BOOST_AUTO_TEST_CASE(ConfigTimeWindowMsTooHigh)
{
    inputConfig.timeWindowMs = 50000;

    BOOST_REQUIRE_EXCEPTION(InputMonitor inputMonitor(inputConfig, InputMonitor::NotifyCallback()),
                            InputMonitorException,
                            WhatEquals("Time window exceeds maximum"));
}

BOOST_AUTO_TEST_CASE(ConfigDeviceFilePathNotExisting)
{
    inputConfig.device = TEST_INPUT_DEVICE + "notExisting";

    BOOST_REQUIRE_EXCEPTION(InputMonitor inputMonitor(inputConfig, InputMonitor::NotifyCallback()),
                            InputMonitorException,
                            WhatEquals("Cannot find a device"));
}

namespace {

void sendNEvents(Fixture& f, unsigned int noOfEventsToSend)
{
    Latch eventLatch;

    InputMonitor inputMonitor(f.inputConfig, [&] {eventLatch.set();});

    int fd = ::open(TEST_INPUT_DEVICE.c_str(), O_WRONLY);
    BOOST_REQUIRE(fd >= 0);

    for (unsigned int i = 0; i < noOfEventsToSend * f.inputConfig.numberOfEvents; ++i) {
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
    BOOST_CHECK(eventLatch.waitForN(noOfEventsToSend, SINGLE_EVENT_TIMEOUT * noOfEventsToSend));

    // Check if no more events are waiting
    BOOST_CHECK(!eventLatch.wait(10));
}

} // namespace

BOOST_AUTO_TEST_CASE(EventOneAtATime)
{
    sendNEvents(*this, 1);
}

BOOST_AUTO_TEST_CASE(EventTenAtATime)
{
    sendNEvents(*this, 10);
}

namespace {

void sendNEventsWithPauses(Fixture& f, unsigned int noOfEventsToSend)
{
    Latch eventLatch;

    InputMonitor inputMonitor(f.inputConfig, [&] {eventLatch.set();});

    int fd = ::open(TEST_INPUT_DEVICE.c_str(), O_WRONLY);
    BOOST_REQUIRE(fd >= 0);

    for (unsigned int i = 0; i < noOfEventsToSend * f.inputConfig.numberOfEvents; ++i) {
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
    BOOST_CHECK(eventLatch.waitForN(noOfEventsToSend, SINGLE_EVENT_TIMEOUT * noOfEventsToSend));

    // Check if no more events are waiting
    BOOST_CHECK(!eventLatch.wait(10));
}

} // namespace

BOOST_AUTO_TEST_CASE(EventOneAtATimeWithPauses)
{
    sendNEventsWithPauses(*this, 1);
}

BOOST_AUTO_TEST_CASE(EventTenAtATimeWithPauses)
{
    sendNEventsWithPauses(*this, 10);
}


BOOST_AUTO_TEST_SUITE_END()
