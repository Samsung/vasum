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

#ifndef COMMON_UTILS_SAME_THREAD_GUARD_HPP
#define COMMON_UTILS_SAME_THREAD_GUARD_HPP

#ifndef NDEBUG
#define ENABLE_SAME_THREAD_GUARD
#endif

#ifdef ENABLE_SAME_THREAD_GUARD
#include <atomic>
#include <cassert>
#endif

namespace utils {

/**
 * Same thread guard.
 * There are two purposes of this guard:
 * - reports invalid assumptions about synchronization needs (only in debug builds)
 * - acts as an annotation in the source code about the thread safety
 *
 * Usage example:
 * ASSERT_SAME_THREAD(workerThreadGuard);
 */
class SameThreadGuard {
public:
#ifdef ENABLE_SAME_THREAD_GUARD
#   define ASSERT_SAME_THREAD(g) assert(g.check())
    SameThreadGuard();

    /**
     * On the first call it remembers the current thread id.
     * On the next call it verifies that current thread is the same as before.
     */
    bool check();

    /**
     * Reset thread id
     */
    void reset();

private:
    std::atomic<unsigned int> mThreadId;

#else // ENABLE_SAME_THREAD_GUARD
#   define ASSERT_SAME_THREAD(g)
    static bool check() {return true;}
    static void reset() {}
#endif // ENABLE_SAME_THREAD_GUARD
};

} // namespace utils


#endif // COMMON_UTILS_SAME_THREAD_GUARD_HPP
