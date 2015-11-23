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

#ifndef CARGO_IPC_EPOLL_THREAD_DISPATCHER_HPP
#define CARGO_IPC_EPOLL_THREAD_DISPATCHER_HPP

#include "cargo-ipc/epoll/event-poll.hpp"
#include "utils/eventfd.hpp"

#include <thread>
#include <atomic>

namespace cargo {
namespace ipc {
namespace epoll {

/**
 * Will dispatch poll events in a newly created thread
 */
class ThreadDispatcher {
public:
    ThreadDispatcher();
    ~ThreadDispatcher();

    EventPoll& getPoll();
private:
    EventPoll mPoll;
    utils::EventFD mStopEvent;
    std::atomic_bool mStopped;
    std::thread mThread;
};

} // namespace epoll
} // namespace ipc
} // namespace cargo

#endif // CARGO_IPC_EPOLL_THREAD_DISPATCHER_HPP
