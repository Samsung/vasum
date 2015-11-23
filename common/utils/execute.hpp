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
 * @brief   Execute utils
 */

#ifndef COMMON_UTILS_EXECUTE_HPP
#define COMMON_UTILS_EXECUTE_HPP

#include <sys/types.h>
#include <vector>
#include <functional>

namespace utils {

/**
 * Execute function - deprecated
 */
bool executeAndWait(const std::function<void()>& func, int& status);

bool executeAndWait(const std::function<void()>& func);


/**
 * Execute binary
 */
///@{
bool executeAndWait(uid_t uid, const char* fname, const char* const* argv, int& status);

bool executeAndWait(const char* fname, const char* const* argv);

bool executeAndWait(const char* fname, const char* const* argv, int& status);

bool executeAndWait(uid_t uid, const std::vector<std::string>& argv, int& status);

bool executeAndWait(uid_t uid, const std::vector<std::string>& argv);

bool executeAndWait(const std::vector<std::string>& argv);
///@}

/**
 * Wait until child processes ends
 */
bool waitPid(pid_t pid, int& status);

} // namespace utils


#endif // COMMON_UTILS_EXECUTE_HPP
