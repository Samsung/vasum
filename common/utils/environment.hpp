/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Michal Witanowski <m.witanowski@samsung.com>
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
 * @author  Michal Witanowski (m.witanowski@samsung.com)
 * @brief   Declaration of environment setup routines that requires root privileges
 */

#ifndef COMMON_UTILS_ENVIRONMENT_HPP
#define COMMON_UTILS_ENVIRONMENT_HPP

#include <string>
#include <vector>
#include <functional>
#include <sys/types.h>


namespace utils {


/**
 * Set supplementary groups to the current process.
 */
bool setSuppGroups(const std::vector<std::string>& groups);

/**
 * Set effective and permitted capabilities on the current process and drop root privileges.
 */
bool dropRoot(uid_t uid, gid_t gid, const std::vector<unsigned int>& caps);

/**
 * Launch binary as root user
 *
 * This function forks, sets UID 0 to child process and calls binary.
 */
bool launchAsRoot(const std::vector<std::string>& argv);

/**
 * Join to namespace
 */
bool joinToNs(int nsPid, int ns);

/**
 * Pass file descriptor from namespace of some process
 */
int passNamespacedFd(int nsPid, int ns, const std::function<int()>& fdFactory);

} // namespace utils


#endif // COMMON_UTILS_ENVIRONMENT_HPP
