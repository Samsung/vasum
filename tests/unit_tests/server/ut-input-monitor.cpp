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
 * @brief   Unit tests of the InputMonitor class
 */

#include "config.hpp"
#include "ut.hpp"

#include "input-monitor.hpp"
#include "input-monitor-config.hpp"
#include "exception.hpp"

#include "utils/glib-loop.hpp"
#include "utils/latch.hpp"

#include <atomic>
#include <chrono>
#include <fcntl.h>
#include <memory>
#include <string>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>


BOOST_AUTO_TEST_SUITE(InputMonitorSuite)

using namespace security_containers;
using namespace security_containers::config;
using namespace security_containers::utils;

namespace {

const char* TEST_INPUT_DEVICE = "/tmp/testInputDevice";

const int EVENT_TYPE = 1;
const int EVENT_CODE = 139;
const int EVENT_BUTTON_RELEASED = 0;
const int EVENT_BUTTON_PRESSED = 1;

const unsigned int ONE_EVENT = 1;
const unsigned int TEN_EVENTS = 10;

const int SINGLE_EVENT_TIMEOUT = 10;

std::atomic<unsigned int> counter(0);


void callbackEmpty()
{
}

void callbackCounter()
{
    counter++;
}

void resetCounter()
{
    counter = 0;
}

unsigned int getCounter()
{
    return counter;
}

/*
 * A wrapper for write() which restarts after EINTR.
 */
ssize_t safeWrite(int fd, const void *buff, size_t len)
{
    size_t totalBytesWritten = 0;

    while (len > 0) {
        ssize_t bytesWritten = write(fd, buff, len);

        if (bytesWritten < 0 && errno == EINTR) {
            continue;
        }

        if (bytesWritten < 0) {
            return bytesWritten;
        }

        if (bytesWritten == 0) {
            return totalBytesWritten;
        }

        buff = (const char *)buff + bytesWritten;
        len -= bytesWritten;
        totalBytesWritten += bytesWritten;
    }

    return totalBytesWritten;
}

class Fixture {
public:
    InputConfig inputConfig;
    utils::ScopedGlibLoop mLoop;
    struct input_event ie[2];

    Fixture() {
        inputConfig.numberOfEvents = 2;
        inputConfig.device = TEST_INPUT_DEVICE;
        inputConfig.code = EVENT_CODE;
        inputConfig.enabled = true;
        inputConfig.timeWindowMs = 500;

        // fill simulated events with init values
        ie[0].time.tv_sec = 946707544;
        ie[0].time.tv_usec = 0;
        ie[0].type = EVENT_TYPE;
        ie[0].code = EVENT_CODE;
        ie[0].value = EVENT_BUTTON_RELEASED;

        ie[1].time.tv_sec = 946707544;
        ie[1].time.tv_usec = 0;
        ie[1].type = 0;
        ie[1].code = 0;
        ie[1].value = 0;

        resetCounter();
        remove(TEST_INPUT_DEVICE);
        mkfifo(TEST_INPUT_DEVICE, 0777);
    }
    ~Fixture() {
        remove(TEST_INPUT_DEVICE);
        resetCounter();
    }
};

} // namespace

BOOST_AUTO_TEST_CASE(Config_OK)
{
    Fixture f;

    BOOST_REQUIRE_NO_THROW(InputMonitor inputMonitor(f.inputConfig, callbackEmpty));
}

BOOST_AUTO_TEST_CASE(Config_numberOfEventsTooHigh)
{
    Fixture f;
    f.inputConfig.numberOfEvents = 100;

    BOOST_REQUIRE_THROW(InputMonitor inputMonitor(f.inputConfig, callbackEmpty),
                        InputMonitorException);
}

BOOST_AUTO_TEST_CASE(Config_timeWindowMsTooHigh)
{
    Fixture f;
    f.inputConfig.timeWindowMs = 50000;

    BOOST_REQUIRE_THROW(InputMonitor inputMonitor(f.inputConfig, callbackEmpty),
                        InputMonitorException);
}

BOOST_AUTO_TEST_CASE(Config_deviceFilePathNotExisting)
{
    Fixture f;
    f.inputConfig.device = std::string(TEST_INPUT_DEVICE) + "notexisting";

    BOOST_REQUIRE_THROW(InputMonitor inputMonitor(f.inputConfig, callbackEmpty),
                        InputMonitorException);
}

// ============================================================================================
// All of the following tests are based on the (almost) the same scenario:
// 1) set up input monitor (create InputMonitor instance + register callback)
// 2) simulate an event(s)
//    a) prepare an example sequence of event(s) to trigger the callback (one or more times,
//       depending on the test purpose)
//    b) send it to the device
// 3) verify that callback was triggered (one or more times accordingly)
//
// Button press + release events are simulated based on logs gathered from working system.
// Example log:
//
//           // button pressed
//           Event detected [gpio-keys.4]:
//             time: 946691112.844176 sec
//             type, code, value: 1, 139, 1
//           Event detected [gpio-keys.4]:
//             time: 946691112.844176 sec
//             type, code, value: 0, 0, 0
//
//           // button released
//           Event detected [gpio-keys.4]:
//             time: 946691113.384301 sec
//             type, code, value: 1, 139, 0
//           Event detected [gpio-keys.4]:
//             time: 946691113.384301 sec
//             type, code, value: 0, 0, 0
//
//
// ============================================================================================

void sendEvents(unsigned int noOfEvents)
{
    Fixture f;
    Latch eventLatch;

    std::unique_ptr<InputMonitor> inputMonitor;
    BOOST_REQUIRE_NO_THROW(inputMonitor.reset(new InputMonitor(f.inputConfig,
            [&] { callbackCounter();
                  if (getCounter() == noOfEvents) {
                      eventLatch.set();
                  }
                })));

    int fd = open(TEST_INPUT_DEVICE, O_WRONLY);

    for (unsigned int i = 0; i < noOfEvents * f.inputConfig.numberOfEvents; ++i) {
        // button pressed event
        f.ie[0].value = EVENT_BUTTON_PRESSED;
        f.ie[0].time.tv_usec += 5;
        f.ie[1].time.tv_usec += 5;
        ssize_t ret = safeWrite(fd, f.ie, 2 * sizeof(struct input_event));
        BOOST_CHECK(ret > 0);

        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        // button released event
        f.ie[0].value = EVENT_BUTTON_RELEASED;
        f.ie[0].time.tv_usec += 5;
        f.ie[1].time.tv_usec += 5;
        ret = safeWrite(fd, f.ie, 2 * sizeof(struct input_event));
        BOOST_CHECK(ret > 0);
    }

    close(fd);

    BOOST_CHECK(eventLatch.wait(SINGLE_EVENT_TIMEOUT * noOfEvents));

    // extra time to make sure, that no more events are read
    std::this_thread::sleep_for(std::chrono::milliseconds(SINGLE_EVENT_TIMEOUT));

    BOOST_CHECK(getCounter() == noOfEvents);
}

BOOST_AUTO_TEST_CASE(Event_oneAtATime)
{
    sendEvents(ONE_EVENT);
}

BOOST_AUTO_TEST_CASE(Event_tenAtATime)
{
    sendEvents(TEN_EVENTS);
}

void sendEventsWithPauses(unsigned int noOfEvents)
{
    Fixture f;
    Latch eventLatch;

    std::unique_ptr<InputMonitor> inputMonitor;
    BOOST_REQUIRE_NO_THROW(inputMonitor.reset(new InputMonitor(f.inputConfig,
            [&] { callbackCounter();
                  if (getCounter() == noOfEvents) {
                      eventLatch.set();
                  }
                })));

    int fd = open(TEST_INPUT_DEVICE, O_WRONLY);

    for (unsigned int i = 0; i < noOfEvents * f.inputConfig.numberOfEvents; ++i) {
        // button pressed event
        f.ie[0].value = EVENT_BUTTON_PRESSED;
        f.ie[0].time.tv_usec += 5;
        f.ie[1].time.tv_usec += 5;
        ssize_t ret = write(fd, f.ie, 2); // send first two bytes
        BOOST_CHECK(ret > 0);

        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        ret = write(fd, &reinterpret_cast<char *>(f.ie)[2], 2 * sizeof(struct input_event) - 2); // send the remaining part
        BOOST_CHECK(ret > 0);

        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        // button released event
        f.ie[0].value = EVENT_BUTTON_RELEASED;
        f.ie[0].time.tv_usec += 5;
        f.ie[1].time.tv_usec += 5;
        ret = write(fd, f.ie, 2 * sizeof(struct input_event));
        BOOST_CHECK(ret > 0);
    }

    close(fd);

    BOOST_CHECK(eventLatch.wait(SINGLE_EVENT_TIMEOUT * noOfEvents));

    // extra time to make sure, that no more events are read
    std::this_thread::sleep_for(std::chrono::milliseconds(SINGLE_EVENT_TIMEOUT));

    BOOST_CHECK(getCounter() == noOfEvents);
}

BOOST_AUTO_TEST_CASE(Event_oneAtATimeWithPauses)
{
    sendEventsWithPauses(ONE_EVENT);
}

BOOST_AUTO_TEST_CASE(Event_tenAtATimeWithPauses)
{
    sendEventsWithPauses(TEN_EVENTS);
}


BOOST_AUTO_TEST_SUITE_END()
