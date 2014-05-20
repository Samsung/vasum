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
 * @brief   C++ wrapper for glib input monitor
 */

#ifndef SERVER_INPUT_MONITOR_HPP
#define SERVER_INPUT_MONITOR_HPP


#include "input-monitor-config.hpp"

#include <functional>
#include <glib.h>
#include <linux/input.h>
#include <sys/time.h>


namespace security_containers {


class InputMonitor {
public:
    typedef std::function<void()> InputNotifyCallback;
    InputMonitor(const InputConfig&, const InputNotifyCallback&);
    ~InputMonitor();

private:
    static const int MAX_NUMBER_OF_EVENTS = 5;
    static const int MAX_TIME_WINDOW_SEC = 10;
    static const int KEY_PRESSED = 1;
    static const int DEVICE_NAME_LENGTH = 256;
    static const std::string DEVICE_DIR;
    struct timeval mEventTime[MAX_NUMBER_OF_EVENTS];
    GIOChannel *mChannel;
    InputConfig mConfig;

    // External callback to be registered at InputMonitor instance
    InputNotifyCallback mNotifyCallback;

    // Internal callback to be registered at glib g_io_add_watch()
    static gboolean readDeviceCallback(GIOChannel *, GIOCondition, gpointer);
    std::string findDeviceNode(const std::string&);
    bool detectExpectedEvent(const struct input_event&);
    void readDevice(GIOChannel *);
    void resetEventsTime();
};

} // namespace security_containers


#endif // SERVER_INPUT_MONITOR_HPP
