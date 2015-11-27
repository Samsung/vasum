/*
 *  Copyright (C) 2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License version 2.1 as published by the Free Software Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

/**
 * @file
 * @author  Jan Olszak (j.olszak@samsung.com)
 * @brief   file system handling routines
 */

#ifndef LXCPP_FILESYSTEM_HPP
#define LXCPP_FILESYSTEM_HPP

#include <sys/types.h>
#include <string>
#include <cstdio>
#include <mntent.h>

namespace lxcpp {

void mount(const std::string& source,
           const std::string& target,
           const std::string& filesystemtype,
           unsigned long mountflags,
           const std::string& data);

void umount(const std::string& path, const int flags = 0);

bool exists(const std::string& path, mode_t mode = 0);

bool isMountPoint(const std::string& path);

/**
 * Detect whether path is mounted as MS_SHARED.
 * Parses /proc/self/mountinfo
 *
 * @param path mount point
 * @return is the mount point shared
 */
bool isMountPointShared(const std::string& path);

FILE *setmntent(const std::string& filename, const std::string& type);

void umountSubtree(const std::string& prefix);

void fchdir(int fd);

void chdir(const std::string& path);

void mkdir(const std::string& path, mode_t mode);

void rmdir(const std::string& path);

void mknod(const std::string& path, mode_t mode, dev_t dev);

void chmod(const std::string& path, mode_t mode);

void chown(const std::string& path, uid_t owner, gid_t group);

void touch(const std::string& path, mode_t mode);

void symlink(const std::string& target, const std::string& linkpath);

void makeNode(const std::string& path, mode_t mode, dev_t dev);

void pivotRoot(const std::string& new_root, const std::string& put_old);

void containerChownRoot(const std::string& path, const struct UserNSConfig& config);

} // namespace lxcpp

#endif // LXCPP_FILESYSTEM_HPP
