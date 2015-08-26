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

#include "lxcpp/filesystem.hpp"
#include "lxcpp/exception.hpp"
#include "lxcpp/process.hpp"

#include "utils/paths.hpp"
#include "utils/exception.hpp"
#include "logger/logger.hpp"

#include <sstream>
#include <fstream>
#include <iterator>
#include <sys/mount.h>
#include <sys/stat.h>
#include <unistd.h>


namespace lxcpp {

void mount(const std::string& source,
           const std::string& target,
           const std::string& filesystemtype,
           unsigned long mountflags,
           const std::string& data)
{
    if (-1 == ::mount(source.c_str(),
                      target.c_str(),
                      filesystemtype.c_str(),
                      mountflags,
                      data.c_str())) {
        const std::string msg = "mount() failed: src:" + source
                                + ", tgt: " + target
                                + ", filesystemtype: " + filesystemtype
                                + ", mountflags: " + std::to_string(mountflags)
                                + ", data: " + data
                                + ", msg: " + utils::getSystemErrorMessage();
        LOGE(msg);
        throw FileSystemSetupException(msg);
    }
}

void umount(const std::string& path, const int flags)
{
    if (-1 == ::umount2(path.c_str(), flags)) {
        const std::string msg = "umount() failed: '" + path + "': " + utils::getSystemErrorMessage();
        LOGD(msg);
        throw FileSystemSetupException(msg);
    }
}

bool isMountPoint(const std::string& path)
{
    std::string parentPath = utils::dirName(path);

    struct stat s1, s2;
    if (-1 == ::stat(path.c_str(), &s1)) {
        const std::string msg = "stat() failed: " + path + ": " + utils::getSystemErrorMessage();
        LOGE(msg);
        throw FileSystemSetupException(msg);
    }

    if (-1 == ::stat(parentPath.c_str(), &s2)) {
        const std::string msg = "stat() failed: " + parentPath + ": " + utils::getSystemErrorMessage();
        LOGE(msg);
        throw FileSystemSetupException(msg);
    }

    return s1.st_dev != s2.st_dev;
}

bool isMountPointShared(const std::string& path)
{
    std::ifstream fileStream("/proc/self/mountinfo");
    if (!fileStream.good()) {
        const std::string msg = "Failed to open /proc/self/mountinfo";
        LOGE(msg);
        throw FileSystemSetupException(msg);
    }

    // Find the line corresponding to the path
    std::string line;
    while (std::getline(fileStream, line).good()) {
        std::istringstream iss(line);
        auto it = std::istream_iterator<std::string>(iss);
        std::advance(it, 4);

        if (it->compare(path)) {
            // Wrong line, different path
            continue;
        }

        // Right line, check if mount point shared
        std::advance(it, 2);
        return it->find("shared:") != std::string::npos;
    }

    // Path not found
    return false;
}

void fchdir(int fd)
{
    if(-1 == ::fchdir(fd)) {
        const std::string msg = "fchdir() failed: " + utils::getSystemErrorMessage();
        LOGE(msg);
        throw FileSystemSetupException(msg);
    }
}

void chdir(const std::string& path)
{
    if(-1 == ::chdir(path.c_str())) {
        const std::string msg = "chdir() failed: " + utils::getSystemErrorMessage();
        LOGE(msg);
        throw FileSystemSetupException(msg);
    }
}

} // namespace lxcpp