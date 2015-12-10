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

#include "config.hpp"

#include "utils/glib-loop.hpp"
#include "utils/callback-wrapper.hpp"

#include <atomic>
#include <cassert>
#include <glib-object.h>

namespace utils {

namespace {
std::atomic_bool gLoopPresent(false);
}


ScopedGlibLoop::ScopedGlibLoop()
    : mLoop(g_main_loop_new(NULL, FALSE), g_main_loop_unref)
{
    if (gLoopPresent.exchange(true)) {
        // only one loop per process
        assert(0 && "Loop is already running");
    }
#if !GLIB_CHECK_VERSION(2,36,0)
    g_type_init();
#endif
    mLoopThread = std::thread(g_main_loop_run, mLoop.get());
}

ScopedGlibLoop::~ScopedGlibLoop()
{
    // ensure loop is running (avoid race condition when stop is called to early)
    while (!g_main_loop_is_running(mLoop.get())) {
        std::this_thread::yield();
    }
    //stop loop and wait
    g_main_loop_quit(mLoop.get());
    mLoopThread.join();
    gLoopPresent = false;
}

void Glib::addTimerEvent(const unsigned int intervalMs,
                         const OnTimerEventCallback& callback,
                         const CallbackGuard& guard)
{
    g_timeout_add_full(G_PRIORITY_DEFAULT,
                       intervalMs,
                       &Glib::onTimerEvent,
                       utils::createCallbackWrapper(callback, guard.spawn()),
                       &utils::deleteCallbackWrapper<OnTimerEventCallback>);
}

gboolean Glib::onTimerEvent(gpointer data)
{
    const OnTimerEventCallback& callback = getCallbackFromPointer<OnTimerEventCallback>(data);
    if (callback) {
        return (gboolean)callback();
    }
    return FALSE;
}


} // namespace utils
