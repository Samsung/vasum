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
 * @brief   C++ wrapper for glib input monitor
 */

#ifndef SERVER_INPUT_MONITOR_HPP
#define SERVER_INPUT_MONITOR_HPP

#include "input-monitor-config.hpp"
#include "utils/callback-guard.hpp"

#include <linux/input.h>
#include <sys/time.h>
#include <glib.h>

#include <functional>
#include <string>
#include <list>


namespace vasum {

class InputMonitor {
public:
    typedef std::function<void()> NotifyCallback;

    InputMonitor(const InputConfig& inputConfig,
                 const NotifyCallback& notifyCallback);
    ~InputMonitor();

private:
    typedef std::function<void(GIOChannel* gio)> ReadDeviceCallback;

    InputConfig mConfig;
    NotifyCallback mNotifyCallback;

    std::list<struct timeval> mEventTimes;
    GIOChannel* mChannelPtr;

    std::string getDevicePath() const;
    void createGIOChannel(const std::string& devicePath);

    // Internal callback to be registered at glib g_io_add_watch()
    static gboolean readDeviceCallback(GIOChannel*, GIOCondition, gpointer);
    bool isExpectedEventSequence(const struct input_event&);
    void readDevice(GIOChannel*);
    utils::CallbackGuard mGuard;
    guint mSourceId;
};

} // namespace vasum


#endif // SERVER_INPUT_MONITOR_HPP
