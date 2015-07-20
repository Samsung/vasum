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
 * @brief   Synchronization latch
 */

#ifndef COMMON_UTILS_LATCH_HPP
#define COMMON_UTILS_LATCH_HPP

#include <mutex>
#include <condition_variable>


namespace utils {


/**
 * A synchronization aid that allows one thread to wait until
 * an operation being performed in other thread completes.
 * It has a similar function as std::promise<void> but allows
 * multiple calls to set.
 */
class Latch {
public:
    Latch();

    /**
     * Sets an event occurred.
     */
    void set();

    /**
     * Waits for a single occurrence of event.
     */
    void wait();

    /**
     * Waits for a single occurrence of event with timeout.
     *
     * @param timeoutMs  timeout in ms to wait for
     * @return           false on timeout
     */
    bool wait(const unsigned int timeoutMs);

    /**
     * Waits for @n occurrences of event.
     *
     * @param n number of occurrences to wait for
     */
    void waitForN(const unsigned int n);

    /**
     * Waits for @n occurrences of event with timeout.
     *
     * @param n          number of occurrences to wait for
     * @param timeoutMs  timeout in ms to wait for
     * @return           false on timeout
     */
    bool waitForN(const unsigned int n, const unsigned int timeoutMs);

    /**
     * Check if there are no pending events.
     */
    bool empty();
private:
    std::mutex mMutex;
    std::condition_variable mCondition;
    unsigned int mCount;
};


} // namespace utils


#endif // COMMON_UTILS_LATCH_HPP
