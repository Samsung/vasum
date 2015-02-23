/*
 *  Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Piotr Bartosiewicz <p.bartosiewi@partner.samsung.com>
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
 * @author  Piotr Bartosiewicz (p.bartosiewi@partner.samsung.com)
 * @brief   Thread epoll dispatcher
 */

#ifndef COMMON_UTILS_THREAD_POLL_DISPATCHER_HPP
#define COMMON_UTILS_THREAD_POLL_DISPATCHER_HPP

#include "utils/event-poll.hpp"
#include "utils/eventfd.hpp"

#include <thread>

namespace vasum {
namespace utils {

/**
 * Will dispatch poll events in a newly created thread
 */
class ThreadPollDispatcher {
public:
    ThreadPollDispatcher(EventPoll& poll);
    ~ThreadPollDispatcher();
private:
    EventPoll& mPoll;
    EventFD mStopEvent;
    std::thread mThread;
};

} // namespace utils
} // namespace vasum

#endif // COMMON_UTILS_THREAD_POLL_DISPATCHER_HPP
