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
 * @brief   Miscellaneous helpers for the Glib library
 */

#ifndef COMMON_UTILS_GLIB_UTILS_HPP
#define COMMON_UTILS_GLIB_UTILS_HPP

#include "utils/callback-guard.hpp"

namespace utils {

typedef std::function<void()> VoidCallback;

/**
 * Executes a callback in glib thread (adds an iddle event to glib)
 */
void executeInGlibThread(const VoidCallback& callback, const CallbackGuard& guard);


} // namespace utils

#endif // COMMON_UTILS_GLIB_UTILS_HPP
