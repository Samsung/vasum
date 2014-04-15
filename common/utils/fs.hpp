/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Lukasz Pawelczyk <l.pawelczyk@partner.samsung.com>
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
 * @author  Lukasz Pawelczyk (l.pawelczyk@partner.samsung.com)
 * @brief   File utility functions declaration
 */

#ifndef COMMON_UTILS_FS_HPP
#define COMMON_UTILS_FS_HPP

#include <string>
#include <sys/types.h>


namespace security_containers {
namespace utils {

/**
 * Reads the content of a file
 */
std::string readFileContent(const std::string& path);

/**
 * Removes a file
 */
bool removeFile(const std::string& path);

/**
 * Checks if a directory exists
 */
bool isDirectory(const std::string& path);

/**
 * Creates a directory
 */
bool createDirectory(const std::string& path, mode_t mode);

/**
 * Creates a directory and its parents as needed
 */
bool createDirectories(const std::string& path, mode_t mode);

/**
 * Mounts a tmpfs on given a path
 */
bool mountTmpfs(const std::string& path);

/**
 * Umounts a filesystem
 */
bool umount(const std::string& path);


} // namespace utils
} // namespace security_containers


#endif // COMMON_UTILS_FS_HPP
