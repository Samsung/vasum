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
#include <vector>


namespace security_containers {
namespace utils {

/**
 * Reads the content of a file; Throws exception on error
 */
std::string readFileContent(const std::string& path);

/**
 * Reads the content of a file
 */
bool readFileContent(const std::string& path, std::string& content);

/**
 * Save the content to the file
 */
bool saveFileContent(const std::string& path, const std::string& content);

/**
 * Checks if a char device exists
 */
bool isCharDevice(const std::string& path);

/**
 * List all (including '.' and '..' entries) dir entries
 */
bool listDir(const std::string& path, std::vector<std::string>& files);

/**
 * Mounts run as a tmpfs on a given path
 */
bool mountRun(const std::string& path);

/**
 * Umounts a filesystem
 */
bool umount(const std::string& path);

/**
 * Check if given path is a mount point
 */
bool isMountPoint(const std::string& path, bool& result);

/**
 * Checks whether the given paths are under the same mount point
 */
bool hasSameMountPoint(const std::string& path1, const std::string& path2, bool& result);

/**
 * Moves the file either by rename if under the same mount point
 * or by copy&delete if under a different one.
 * The destination has to be a full path including file name.
 */
bool moveFile(const std::string& src, const std::string& dst);


} // namespace utils
} // namespace security_containers


#endif // COMMON_UTILS_FS_HPP
