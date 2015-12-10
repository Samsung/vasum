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

#ifndef COMMON_UTILS_CALLBACK_GUARD_HPP
#define COMMON_UTILS_CALLBACK_GUARD_HPP

#include <memory>


namespace utils {

/**
 * Callback guard.
 * An utility class to control and/or monitor callback lifecycle.
 */
class CallbackGuard {
public:
    typedef std::shared_ptr<void> Tracker;

    /**
     * Creates a guard.
     */
    CallbackGuard();

    /**
     * Waits for all trackers.
     */
    ~CallbackGuard();

    /**
     * Creates a tracker.
     */
    Tracker spawn() const;

    /**
     * Gets trackers count
     */
    long getTrackersCount() const;

    /**
     * Wait for all trackers.
     */
    bool waitForTrackers(const unsigned int timeoutMs);
private:
    class SharedState;
    std::shared_ptr<SharedState> mSharedState;

    CallbackGuard(const CallbackGuard&) = delete;
    CallbackGuard& operator=(const CallbackGuard&) = delete;
};

} // namespace utils


#endif // COMMON_UTILS_CALLBACK_GUARD_HPP
