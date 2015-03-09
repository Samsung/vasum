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

#include "config.hpp"
#include "epoll/event-poll.hpp"
#include "utils/fd-utils.hpp"
#include "utils/exception.hpp"
#include "logger/logger.hpp"

#include <sys/epoll.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

namespace vasum {
namespace epoll {

EventPoll::EventPoll()
    : mPollFD(::epoll_create1(EPOLL_CLOEXEC))
{
    if (mPollFD == -1) {
        LOGE("Failed to create epoll: " << getSystemErrorMessage());
        throw UtilsException("Could not create epoll");
    }
}

EventPoll::~EventPoll()
{
    if (!mCallbacks.empty()) {
        LOGW("Not removed callbacks: " << mCallbacks.size());
        assert(0 && "Not removed callbacks left");
    }
    utils::close(mPollFD);
}

int EventPoll::getPollFD() const
{
    return mPollFD;
}

void EventPoll::addFD(const int fd, const Events events, Callback&& callback)
{
    std::lock_guard<Mutex> lock(mMutex);

    if (mCallbacks.find(fd) != mCallbacks.end()) {
        LOGW("Already added fd: " << fd);
        throw UtilsException("FD already added");
    }

    if (!addFDInternal(fd, events)) {
        throw UtilsException("Could not add fd");
    }

    mCallbacks.insert({fd, std::make_shared<Callback>(std::move(callback))});
    LOGT("Callback added for " << fd);
}

void EventPoll::removeFD(const int fd)
{
    std::lock_guard<Mutex> lock(mMutex);

    auto iter = mCallbacks.find(fd);
    if (iter == mCallbacks.end()) {
        LOGW("Failed to remove nonexistent fd: " << fd);
        throw UtilsException("FD does not exist");
    }
    mCallbacks.erase(iter);
    removeFDInternal(fd);
    LOGT("Callback removed for " << fd);
}

bool EventPoll::dispatchIteration(const int timeoutMs)
{
    for (;;) {
        epoll_event event;
        int num = epoll_wait(mPollFD, &event, 1, timeoutMs);
        if (num == 0) {
            return false; // timeout
        }
        if (num < 0) {
            if (errno == EINTR) {
                continue;
            }
            LOGE("Failed to wait on epoll: " << getSystemErrorMessage());
            throw UtilsException("Could not wait for event");
        }

        // callback could be removed in the meantime, so be careful, find it inside lock
        std::lock_guard<Mutex> lock(mMutex);
        auto iter = mCallbacks.find(event.data.fd);
        if (iter == mCallbacks.end()) {
            continue;
        }

        // add ref because removeFD(self) can be called inside callback
        std::shared_ptr<Callback> callback(iter->second);
        return (*callback)(event.data.fd, event.events);
    }
}

void EventPoll::dispatchLoop()
{
    while (dispatchIteration(-1)) {}
}

bool EventPoll::addFDInternal(const int fd, const Events events)
{
    epoll_event event;
    memset(&event, 0, sizeof(event));
    event.events = events;
    event.data.fd = fd;

    if (epoll_ctl(mPollFD, EPOLL_CTL_ADD, fd, &event) == -1) {
        LOGE("Failed to add fd to poll: " << getSystemErrorMessage());
        return false;
    }
    return true;
}

void EventPoll::removeFDInternal(const int fd)
{
    if (epoll_ctl(mPollFD, EPOLL_CTL_DEL, fd, NULL) == -1) {
        assert(errno != EBADF); // always removeFD before closing fd locally!
                                // this is important because linux will reuse this fd number
        LOGE("Failed to remove fd from poll: " << getSystemErrorMessage());
    }
}

} // namespace epoll
} // namespace vasum
