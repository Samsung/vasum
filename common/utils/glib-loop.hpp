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
 * @brief   C++ wrapper of glib main loop
 */

#ifndef COMMON_UTILS_GLIB_LOOP_HPP
#define COMMON_UTILS_GLIB_LOOP_HPP

#include "utils/callback-guard.hpp"

#include <thread>
#include <memory>
#include <glib.h>


namespace utils {


/**
 * Glib loop controller. Loop is running in separate thread.
 */
class ScopedGlibLoop {
public:
    /**
     * Starts a loop in separate thread.
     */
    ScopedGlibLoop();

    /**
     * Stops loop and waits for a thread.
     */
    ~ScopedGlibLoop();

private:
    std::unique_ptr<GMainLoop, void(*)(GMainLoop*)> mLoop;
    std::thread mLoopThread;
};

/**
 * Miscellaneous helpers for the Glib library
 */
class Glib {
public:
    /**
     * A user provided function that will be called succesively after an interval has passed.
     *
     * Return true if the callback is supposed to be called further,
     *        false if the callback is not to be called anymore and be destroyed.
     */
    typedef std::function<bool()> OnTimerEventCallback;

    /**
     * Adds a timer event to the glib main loop.
     */
    static void addTimerEvent(const unsigned int intervalMs,
                              const OnTimerEventCallback& callback,
                              const CallbackGuard& guard);

private:
    static gboolean onTimerEvent(gpointer data);
};

} // namespace utils


#endif // COMMON_UTILS_GLIB_LOOP_HPP
