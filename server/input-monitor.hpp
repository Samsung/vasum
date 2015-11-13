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
#include "cargo-ipc/epoll/event-poll.hpp"

#include <linux/input.h>
#include <sys/time.h>

#include <string>
#include <list>
#include <mutex>


namespace vasum {

class ZonesManager;

class InputMonitor {
public:
    InputMonitor(cargo::ipc::epoll::EventPoll& eventPoll,
                 const InputConfig& inputConfig,
                 ZonesManager* zonesManager);
    ~InputMonitor();

    void start();
    void stop();
private:
    typedef std::mutex Mutex;

    InputConfig mConfig;
    ZonesManager* mZonesManager;
    int mFd;
    cargo::ipc::epoll::EventPoll& mEventPoll;
    std::list<struct timeval> mEventTimes;
    std::string mDevicePath;
    Mutex mMutex;

    std::string getDevicePath() const;
    void setHandler(const std::string& devicePath);
    void handleInternal(int fd, cargo::ipc::epoll::Events events);
    void leaveDevice();
    bool isExpectedEventSequence(const struct input_event&);
};

} // namespace vasum


#endif // SERVER_INPUT_MONITOR_HPP
