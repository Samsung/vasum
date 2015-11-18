/*
*  Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
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
 * @brief   Signalfd wrapper
 */

#ifndef COMMON_UTILS_SIGNALFD_HPP
#define COMMON_UTILS_SIGNALFD_HPP

#include "cargo-ipc/epoll/event-poll.hpp"

#include <csignal>
#include <sys/signalfd.h>

#include <functional>
#include <mutex>
#include <unordered_map>
#include <memory>
#include <vector>

namespace utils {

/**
 * SignalFD takes control over handling signals
 * sent to the thread.
 *
 * It should be the only place where signal masks are modified.
 */
class SignalFD {
public:
    typedef std::function<void(struct ::signalfd_siginfo&)> Callback;

    SignalFD(cargo::ipc::epoll::EventPoll& eventPoll);
    virtual ~SignalFD();

    SignalFD(const SignalFD& signalfd) = delete;
    SignalFD& operator=(const SignalFD&) = delete;

    /**
     * Add a callback for a specified signal.
     * Blocks the async signal handler if it's not already blocked.
     *
     * @param sigNum number of the signal
     * @param callback handler callback
     */
    void setHandler(const int sigNum, const Callback&& callback);

    /**
     * @return signal file descriptor
     */
    int getFD() const;

private:
    typedef std::unique_lock<std::mutex> Lock;

    int mFD;
    ::sigset_t mSet;
    std::mutex mMutex;
    cargo::ipc::epoll::EventPoll& mEventPoll;
    std::unordered_map<int, Callback> mCallbacks;
    std::vector<int> mBlockedSignals;

    void handleInternal();
};

} // namespace utils

#endif // COMMON_UTILS_SIGNALFD_HPP
