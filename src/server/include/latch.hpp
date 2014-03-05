/*
 *  Copyright (c) 2000 - 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Bumjin Im <bj.im@samsung.com>
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
 * @file    latch.hpp
 * @author  Piotr Bartosiewicz (p.bartosiewi@partner.samsung.com)
 * @brief   Synchronization latch
 */

#ifndef LATCH_HPP
#define LATCH_HPP

#include <mutex>
#include <condition_variable>

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
     * Waits with timeout.
     * @return false on timeout
     */
    bool wait(const unsigned int timeoutMs);

    /**
     * Check if there are no pending events.
     */
    bool empty();
private:
    std::mutex mMutex;
    std::condition_variable mCondition;
    int mCount;
};

#endif // LATCH_HPP
