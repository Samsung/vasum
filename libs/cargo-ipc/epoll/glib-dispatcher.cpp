/*
 *  Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
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
 * @brief   glib epoll dispatcher
 */

#include "config.hpp"

#include "cargo-ipc/epoll/glib-dispatcher.hpp"
#include "utils/callback-wrapper.hpp"

namespace cargo {
namespace ipc {
namespace epoll {

GlibDispatcher::GlibDispatcher()
{
    mChannel = g_io_channel_unix_new(mPoll.getPollFD());

    auto dispatchCallback = [this]() {
        mPoll.dispatchIteration(0);
    };

    auto cCallback = [](GIOChannel*, GIOCondition, gpointer data) -> gboolean {
        utils::getCallbackFromPointer<decltype(dispatchCallback)>(data)();
        return TRUE;
    };

    mWatchId = g_io_add_watch_full(mChannel,
                                   G_PRIORITY_DEFAULT,
                                   G_IO_IN,
                                   cCallback,
                                   utils::createCallbackWrapper(dispatchCallback, mGuard.spawn()),
                                   &utils::deleteCallbackWrapper<decltype(dispatchCallback)>);
}

GlibDispatcher::~GlibDispatcher()
{
    g_source_remove(mWatchId);
    g_io_channel_unref(mChannel);
    // mGuard destructor will wait for full unregister of dispatchCallback
}

EventPoll& GlibDispatcher::getPoll()
{
    return mPoll;
}

} // namespace epoll
} // namespace ipc
} // namespace cargo
