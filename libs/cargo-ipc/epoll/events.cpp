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
 * @brief   Epoll events
 */

#include "config.hpp"

#include "cargo-ipc/epoll/events.hpp"

#include <sstream>

namespace cargo {
namespace ipc {
namespace epoll {

namespace {

std::string eventToString(Events event)
{
    switch (event) {
    case EPOLLIN: return "IN";
    case EPOLLOUT: return "OUT";
    case EPOLLERR: return "ERR";
    case EPOLLHUP: return "HUP";
    case EPOLLRDHUP: return "RDHUP";
    default:
        std::ostringstream ss;
        ss << "0x" << std::hex << event;
        return ss.str();
    }
}

} // namespace

std::string eventsToString(Events events)
{
    if (events == 0) {
        return "<NONE>";
    }
    std::string ret;
    for (unsigned int i = 0; i<32; ++i) {
        Events event = 1u << i;
        if (events & event) {
            if (!ret.empty()) {
                ret.append(", ");
            }
            ret.append(eventToString(event));
        }
    }
    return ret;
}

} // namespace epoll
} // namespace ipc
} // namespace cargo
