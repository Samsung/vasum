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

#ifndef COMMON_UTILS_GLIB_POLL_DISPATCHER_HPP
#define COMMON_UTILS_GLIB_POLL_DISPATCHER_HPP

#include "utils/event-poll.hpp"
#include "utils/callback-guard.hpp"

#include <gio/gio.h>

namespace vasum {
namespace utils {

/**
 * Will dispatch poll events in glib thread
 */
class GlibPollDispatcher {
public:
    GlibPollDispatcher(EventPoll& poll);
    ~GlibPollDispatcher();
private:
    CallbackGuard mGuard;
    GIOChannel* mChannel;
    guint mWatchId;
};


} // namespace utils
} // namespace vasum

#endif // COMMON_UTILS_GLIB_POLL_DISPATCHER_HPP
