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

#ifndef CARGO_IPC_EPOLL_EVENT_POLL_HPP
#define CARGO_IPC_EPOLL_EVENT_POLL_HPP

#include "cargo-ipc/epoll/events.hpp"

#include <functional>
#include <mutex>
#include <unordered_map>
#include <memory>

namespace cargo {
namespace ipc {
namespace epoll {

/**
 * @brief This class waits on registered file descriptor for events.
 * It uses epoll mechanism.
 *
 * @see cargo::ipc::epoll::Events
 *
 * @ingroup Types
 */
class EventPoll {
public:
    /**
     * Generic function type used as callback for epoll events.
     *
     * @param   fd      descriptor that triggered the event
     * @param   events  event mask that occured
     * @see cargo::ipc::epoll::Events
     */
    typedef std::function<void(int fd, Events events)> Callback;

    /**
     * Constructs the EventPoll and initializes the underlaying epoll mechanism.
     * @throw UtilsException thrown if epoll initialization failed
     */
    EventPoll();
    ~EventPoll();

    /**
     * Returns epoll handle.
     * @return handle or -1 if not initialized
     */
    int getPollFD() const;

    /**
     * Add descriptor and it's watched events.
     *
     * @param fd                descriptor to watch
     * @param events            events to associate with the descriptor
     * @param callback          callback to call once the event occurs
     * @throw UtilsException    thrown if descriptor already registered or add fail
     * @see cargo::ipc::epoll::Events
     */
    void addFD(const int fd, const Events events, Callback&& callback);

    /**
     * Modify watched events for descriptor.
     * @param fd                watched descriptor, already registered
     * @param events            events to associate with the descriptor
     * @throw UtilsException    if descriptor not found or epoll modify fail
     * @see cargo::ipc::epoll::Events
     */
    void modifyFD(const int fd, const Events events);

    /**
     * Remove descriptor from the watch list.
     * @param fd                watched descriptor
     */
    void removeFD(const int fd);

    /**
     * Wait for events on descriptor on the watch list.
     * Dispatch at most one signaled FD.
     *
     * @param timeoutMs       how long should wait in case of no pending events
     *                        (0 - return immediately, -1 - wait forever)
     * @throw UtilsException  if epoll_wait fails
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
} // namespace cargo

#endif // CARGO_IPC_EPOLL_EVENT_POLL_HPP
