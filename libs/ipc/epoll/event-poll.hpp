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
 * @brief   C++ epoll wrapper
 */

#ifndef COMMON_EPOLL_EVENT_POLL_HPP
#define COMMON_EPOLL_EVENT_POLL_HPP

#include "ipc/epoll/events.hpp"

#include <functional>
#include <mutex>
#include <unordered_map>
#include <memory>

namespace ipc {
namespace epoll {

class EventPoll {
public:
    typedef std::function<void(int fd, Events events)> Callback;

    EventPoll();
    ~EventPoll();

    int getPollFD() const;

    void addFD(const int fd, const Events events, Callback&& callback);
    void modifyFD(const int fd, const Events events);
    void removeFD(const int fd);

    /**
     * Dispatch at most one signalled FD
     * @param timeoutMs how long should wait in case of no pending events
     *        (0 - return immediately, -1 - wait forever)
     * @return false on timeout
     */
    bool dispatchIteration(const int timeoutMs);

private:
    typedef std::recursive_mutex Mutex;

    const int mPollFD;
    Mutex mMutex;
    std::unordered_map<int, std::shared_ptr<Callback>> mCallbacks;

    bool addFDInternal(const int fd, const Events events);
    bool modifyFDInternal(const int fd, const Events events);
    void removeFDInternal(const int fd);
};


} // namespace epoll
} // namespace ipc

#endif // COMMON_EPOLL_EVENT_POLL_HPP
