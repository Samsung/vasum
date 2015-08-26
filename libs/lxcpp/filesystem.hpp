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

#include <string>

namespace lxcpp {

void mount(const std::string& source,
           const std::string& target,
           const std::string& filesystemtype,
           unsigned long mountflags,
           const std::string& data);

void umount(const std::string& path, const int flags);

bool isMountPoint(const std::string& path);

/**
 * Detect whether path is mounted as MS_SHARED.
 * Parses /proc/self/mountinfo
 *
 * @param path mount point
 * @return is the mount point shared
 */
bool isMountPointShared(const std::string& path);

void fchdir(int fd);

void chdir(const std::string& path);

} // namespace lxcpp

#endif // LXCPP_FILESYSTEM_HPP