/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
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
 * @brief   Callback guard
 */

#include "config.hpp"

#include "utils/callback-guard.hpp"
#include "logger/logger.hpp"

#include <mutex>
#include <condition_variable>
#include <cassert>


namespace utils {

// Reference counting class like shared_ptr but with the ability to wait for it.
class CallbackGuard::SharedState {
public:
    SharedState() : mCounter(0) {}

    void inc()
    {
        std::unique_lock<std::mutex> lock(mMutex);
        ++mCounter;
    }

    void dec()
    {
        std::unique_lock<std::mutex> lock(mMutex);
        --mCounter;
        mEmptyCondition.notify_all();
    }

    long size()
    {
        std::unique_lock<std::mutex> lock(mMutex);
        return mCounter;
    }

    bool wait(const unsigned int timeoutMs)
    {
        std::unique_lock<std::mutex> lock(mMutex);
        return mEmptyCondition.wait_for(lock,
                                        std::chrono::milliseconds(timeoutMs),
                                        [this] {return mCounter == 0;});
    }
private:
    std::mutex mMutex;
    std::condition_variable mEmptyCondition;
    long mCounter;
};

namespace {

template<class SharedState>
class TrackerImpl {
public:
    TrackerImpl(const std::shared_ptr<SharedState>& sharedState)
        : mSharedState(sharedState)
    {
        mSharedState->inc();
    }

    ~TrackerImpl()
    {
        mSharedState->dec();
    }
private:
    std::shared_ptr<SharedState> mSharedState;
};

// Relatively big timeout in case of deadlock.
// This timeout should never be exceeded with
// a properly written code.
const unsigned int TIMEOUT = 5000;

} // namespace


CallbackGuard::CallbackGuard()
    : mSharedState(new SharedState())
{
}

CallbackGuard::~CallbackGuard()
{
    if (!waitForTrackers(TIMEOUT)) {
        LOGE("==== DETECTED INVALID CALLBACK USE ====");
        assert(0 && "Invalid callback use");
    }
}

CallbackGuard::Tracker CallbackGuard::spawn() const
{
    return Tracker(new TrackerImpl<SharedState>(mSharedState));
}

long CallbackGuard::getTrackersCount() const
{
    return mSharedState->size();
}

bool CallbackGuard::waitForTrackers(const unsigned int timeoutMs)
{
    return mSharedState->wait(timeoutMs);
}

} // namespace utils
