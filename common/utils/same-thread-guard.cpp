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
 * @brief   Same thread guard
 */

#include "config.hpp"

#include "utils/same-thread-guard.hpp"

#ifdef ENABLE_SAME_THREAD_GUARD

#include "logger/logger.hpp"
#include "logger/formatter.hpp"

namespace utils {

namespace {

typedef decltype(logger::LogFormatter::getCurrentThread()) ThreadId;
const ThreadId NOT_SET = 0;

ThreadId getCurrentThreadId() {
    // use the same thread id numbering mechanism as in logger
    // to allow analyse accesses in log
    return logger::LogFormatter::getCurrentThread();
}

} // namespace

SameThreadGuard::SameThreadGuard() : mThreadId(NOT_SET)
{
    static_assert(std::is_same<decltype(mThreadId.load()), ThreadId>::value,
                  "thread id type mismatch");
}

bool SameThreadGuard::check()
{
    const ThreadId thisThreadId = getCurrentThreadId();

    ThreadId saved = NOT_SET;
    if (!mThreadId.compare_exchange_strong(saved, thisThreadId) && saved != thisThreadId) {
        LOGE("Detected thread id mismatch; saved: " << saved << "; current: " << thisThreadId);
        return false;
    }
    return true;
}

void SameThreadGuard::reset()
{
    mThreadId.store(NOT_SET);
}

} // namespace utils

#endif // ENABLE_SAME_THREAD_GUARD
