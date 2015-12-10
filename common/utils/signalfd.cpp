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
 * @brief   Eventfd wrapper
 */

#include "config.hpp"

#include "utils/signalfd.hpp"
#include "utils/signal.hpp"
#include "utils/fd-utils.hpp"
#include "utils/exception.hpp"
#include "logger/logger.hpp"

#include <functional>

namespace utils {

SignalFD::SignalFD(cargo::ipc::epoll::EventPoll& eventPoll)
    :mEventPoll(eventPoll)
{
    int error = ::sigemptyset(&mSet);
    if (error == -1) {
        const std::string msg = "Error in sigemptyset: " + getSystemErrorMessage();
        LOGE(msg);
        throw UtilsException(msg);
    }

    mFD = ::signalfd(-1, &mSet, SFD_CLOEXEC);
    if (mFD == -1) {
        const std::string msg = "Error in signalfd: " + getSystemErrorMessage();
        LOGE(msg);
        throw UtilsException(msg);
    }

    mEventPoll.addFD(mFD, EPOLLIN, std::bind(&SignalFD::handleInternal, this));
}

SignalFD::~SignalFD()
{
    mEventPoll.removeFD(mFD);
    utils::close(mFD);

    // Unblock the signals that have been blocked previously, but also eat
    // them if they were pending. It seems that signals are delivered twice,
    // independently for signalfd and async. If we don't eat them before
    // unblocking they will be delivered immediately potentially doing harm.
    for (const int sigNum : mBlockedSignals) {
        waitForSignal(sigNum, 0);

        // Yes, there is a race here between waitForSignal and signalUnlock, but if
        // a signal is sent at this point it's not by us, signalFD is inactive. So
        // if that is the case I'd expect someone to have set some handler already.

        signalUnblock(sigNum);
    }
}

int SignalFD::getFD() const
{
    return mFD;
}

void SignalFD::setHandler(const int sigNum, const Callback&& callback)
{
    Lock lock(mMutex);

    bool isBlocked = isSignalBlocked(sigNum);
    if(!isBlocked) {
        signalBlock(sigNum);
        mBlockedSignals.push_back(sigNum);
    }

    int error = ::sigaddset(&mSet, sigNum);
    if (error == -1) {
        const std::string msg = getSystemErrorMessage();
        LOGE("Error in signalfd: " << msg);
        if(!isBlocked) {
            signalUnblock(sigNum);
            mBlockedSignals.pop_back();
        }
        throw UtilsException("Error in signalfd: " + msg);
    }

    error = ::signalfd(mFD, &mSet, SFD_CLOEXEC);
    if (error != mFD) {
        const std::string msg = getSystemErrorMessage();
        LOGE("Error in signalfd: " << msg);
        if(!isBlocked) {
            signalUnblock(sigNum);
            mBlockedSignals.pop_back();
        }
        throw UtilsException("Error in signalfd: " + msg);
    }

    mCallbacks.insert({sigNum, callback});
}

void SignalFD::handleInternal()
{
    struct ::signalfd_siginfo sigInfo;
    utils::read(mFD, &sigInfo, sizeof(sigInfo));

    LOGT("Got signal: " << sigInfo.ssi_signo);

    {
        Lock lock(mMutex);
        auto it = mCallbacks.find(sigInfo.ssi_signo);
        if (it == mCallbacks.end()) {
            // Meantime the callback was deleted
            LOGE("No callback for signal: " << sigInfo.ssi_signo);
            return;
        }

        it->second(sigInfo);
    }
}

} // namespace utils
