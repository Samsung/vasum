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
 * @file
 * @author  Piotr Bartosiewicz (p.bartosiewi@partner.samsung.com)
 * @brief   C++ wrapper of glib main loop
 */

#include "utils/glib-loop.hpp"

#include <glib.h>


namespace security_containers {
namespace utils {


ScopedGlibLoop::ScopedGlibLoop()
    : mLoop(g_main_loop_new(NULL, FALSE), g_main_loop_unref)
{
    mLoopThread = std::thread([this] {g_main_loop_run(mLoop.get());});
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
}


} // namespace utils
} // namespace security_containers
