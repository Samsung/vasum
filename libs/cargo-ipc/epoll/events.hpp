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

#ifndef CARGO_IPC_EPOLL_EVENTS_HPP
#define CARGO_IPC_EPOLL_EVENTS_HPP

#include <string>
#include <sys/epoll.h> // for EPOLL* constatnts

namespace cargo {
namespace ipc {
namespace epoll {

/**
 * @brief bitmask of EPOLL* constants
 * @ingroup Types
 */
typedef unsigned int Events;

/**
 * Convert event mask into readable string.
 * Values will be comma separated.
 * @param events    event type mask to convert
 * @return string describing the mask
 */
std::string eventsToString(Events events);

} // namespace epoll
} // namespace ipc
} // namespace cargo

#endif // CARGO_IPC_EPOLL_EVENTS_HPP
