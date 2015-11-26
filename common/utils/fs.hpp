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
#include <boost/filesystem.hpp>


namespace utils {

/**
 * Reads the content of file stream (no seek); Throws exception on error
 */
std::string readFileStream(const std::string& path);

/**
 * Reads the content of file stream (no seek)
 */
bool readFileStream(const std::string& path, std::string& result);

/**
 * Reads the content of a file (performs seek); Throws exception on error
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
 * Read a line from file
 * Its goal is to read a kernel config files (eg. from /proc, /sys/)
 */
bool readFirstLineOfFile(const std::string& path, std::string& ret);

/**
 * Remove file
 */
bool removeFile(const std::string& path);

/**
 * Checks if a path exists and points to an expected item type.
 * @return: true if exists and is a directory, false otherwise
 */
bool exists(const std::string& path, int inodeType = 0);

/**
 * Checks if a path exists and points to an expected item type.
 */
void assertExists(const std::string& path, int inodeType = 0);

/**
 * Checks if a char device exists
 */
bool isCharDevice(const std::string& path);

/**
 * Checks if a path exists and points to a directory
 * @return: true if exists and is a directory, false otherwise
 */
bool isDir(const std::string& path);

/**
 * Checks if a path exists and points to a directory
 */
void assertIsDir(const std::string& path);

/**
 * Checks if a path exists and points to a regular file or link
 * @return: true if exists and is a regular file, false otherwise
 */
bool isRegularFile(const std::string& path);

/**
 * Checks if a path exists and points to a regular file or link
 */
void assertIsRegularFile(const std::string& path);

/**
 * Checks if path is absolute
 * @return: true if path is valid absolute path, false otherwise
 */
bool isAbsolute(const std::string& path);

/**
 * Checks if path is absolute
 */
void assertIsAbsolute(const std::string& path);

/**
 * List all (including '.' and '..' entries) dir entries
 */
bool listDir(const std::string& path, std::vector<std::string>& files);

/**
 * Mounts run as a tmpfs on a given path
 */
bool mountRun(const std::string& path);

/**
 * Creates mount point
 */
bool mount(const std::string& source,
           const std::string& target,
           const std::string& filesystemtype,
           unsigned long mountflags,
           const std::string& data);

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

/**
 * Recursively copy contents of src dir to dst dir.
 */
bool copyDirContents(const std::string& src, const std::string& dst);

/**
 * Creates a directory with specific UID, GID and permissions set.
 */
bool createDir(const std::string& path, uid_t uid, uid_t gid, boost::filesystem::perms mode);

/**
 * Recursively creates a directory with specific permissions set.
 */
bool createDirs(const std::string& path, mode_t mode);

/**
 * Creates an empty directory, ready to serve as mount point.
 * Succeeds either if path did not exist and was created successfully, or if already existing dir
 * under the same path is empty and is not a mount point.
 */
bool createEmptyDir(const std::string& path);

/**
 * Creates an empty file
 */
bool createFile(const std::string& path, int flags, mode_t mode);

/**
 * Creates an FIFO special file
 */
bool createFifo(const std::string& path, mode_t mode);

/**
 * Copy an file
 */
bool copyFile(const std::string& src, const std::string& dest);

/**
 * Create hard link
 */
bool createLink(const std::string& src, const std::string& dest);

} // namespace utils


#endif // COMMON_UTILS_FS_HPP
