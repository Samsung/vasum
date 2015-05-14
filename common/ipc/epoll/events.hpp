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

#ifndef COMMON_EPOLL_EVENTS_HPP
#define COMMON_EPOLL_EVENTS_HPP

#include <string>
#include <sys/epoll.h> // for EPOLL* constatnts

namespace ipc {
namespace epoll {

typedef unsigned int Events; ///< bitmask of EPOLL* constants

std::string eventsToString(Events events);

} // namespace epoll
} // namespace ipc

#endif // COMMON_EPOLL_EVENTS_HPP
