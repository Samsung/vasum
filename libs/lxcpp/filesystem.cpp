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

#include "config.hpp"

#include "lxcpp/filesystem.hpp"
#include "lxcpp/exception.hpp"
#include "lxcpp/process.hpp"
#include "lxcpp/userns-config.hpp"

#include "utils/paths.hpp"
#include "utils/text.hpp"
#include "utils/exception.hpp"
#include "utils/fd-utils.hpp"
#include "logger/logger.hpp"

#include <algorithm>
#include <sstream>
#include <fstream>
#include <iterator>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <unistd.h>


namespace {

const int MNT_BUF_SIZE = 1024;

} // namespace


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
        LOGE(msg);
        throw FileSystemSetupException(msg);
    }
}

bool exists(const std::string& path, mode_t mode)
{
    struct stat buf;

    if (::stat(path.c_str(), &buf) < 0) {
        if (errno == ENOENT) {
            return false;
        }

        const std::string msg = "stat() failed: '" + path + "': " + utils::getSystemErrorMessage();
        LOGE(msg);
        throw FileSystemSetupException(msg);
    }

    if (mode && (buf.st_mode & mode) != mode) {
        return false;
    }

    return true;
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

void bindMountFile(const std::string &source, const std::string &target)
{
    LOGD("Bind mounting: " << source << " to: " << target);

    lxcpp::touch(target, 0666);
    lxcpp::mount(source, target, "", MS_BIND, "");
}

FILE *setmntent(const std::string& filename, const std::string& type)
{
    FILE *ret = ::setmntent(filename.c_str(), type.c_str());

    if (NULL == ret) {
        const std::string msg = "setmntent() failed: " + utils::getSystemErrorMessage();
        LOGE(msg);
        throw FileSystemSetupException(msg);
    }

    return ret;
}

void umountSubtree(const std::string& prefix)
{
    std::vector<std::string> mounts;
    struct mntent mntEntry;
    char mntBuf[MNT_BUF_SIZE];

    FILE *procmnt = lxcpp::setmntent("/proc/mounts", "r");

    while(::getmntent_r(procmnt, &mntEntry, mntBuf, sizeof(mntBuf)) != NULL) {
        if(utils::beginsWith(mntEntry.mnt_dir, prefix)) {
            mounts.push_back(mntEntry.mnt_dir);
        }
    }

    ::endmntent(procmnt);

    std::sort(mounts.begin(), mounts.end(),
        [](const std::string &s1, const std::string &s2) {
            return (s1.compare(s2) >= 0);
        }
    );

    for (const auto& mnt: mounts) {
        try {
            lxcpp::umount(mnt);
        } catch(...) {
            LOGD("Failed to umount: " << mnt << " trying to detach: " << *mounts.rbegin());
            lxcpp::umount(*mounts.rbegin(), MNT_DETACH);
            lxcpp::umount(*mounts.rbegin());
            break;
        }
    }
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
        const std::string msg = "chdir() failed: " + path + ": "  + utils::getSystemErrorMessage();
        LOGE(msg);
        throw FileSystemSetupException(msg);
    }
}

void mkdir(const std::string& path, mode_t mode)
{
    if(-1 == ::mkdir(path.c_str(), mode)) {
        if (errno == EEXIST)
            return;

        const std::string msg = "mkdir() failed: " + path + ": "  + utils::getSystemErrorMessage();
        LOGE(msg);
        throw FileSystemSetupException(msg);
    }
}

void rmdir(const std::string& path)
{
    if (-1 == ::rmdir(path.c_str()) && errno != ENOENT) {
        const std::string msg = "rmdir() failed: " + path + ": " + utils::getSystemErrorMessage();
        LOGE(msg);
        throw FileSystemSetupException(msg);
    }
}

void mknod(const std::string& path, mode_t mode, dev_t dev)
{
    if (::mknod(path.c_str(), mode, dev) < 0) {
        const std::string msg = "mknod() failed: " + path + ": "  + utils::getSystemErrorMessage();
        LOGE(msg);
        throw FileSystemSetupException(msg);
    }
}

void chmod(const std::string& path, mode_t mode)
{
    if (::chmod(path.c_str(), mode) < 0) {
        const std::string msg = "chmod() failed: " + path + ": "  + utils::getSystemErrorMessage();
        LOGE(msg);
        throw FileSystemSetupException(msg);
    }
}

void chown(const std::string& path, uid_t owner, gid_t group)
{
    if (::chown(path.c_str(), owner, group) < 0) {
        const std::string msg = "chown() failed: " + path + ": "  + utils::getSystemErrorMessage();
        LOGE(msg);
        throw FileSystemSetupException(msg);
    }
}

void symlink(const std::string& target, const std::string& linkpath)
{
    if (::symlink(target.c_str(), linkpath.c_str()) < 0) {
        const std::string msg = "symlink() failed: " + utils::getSystemErrorMessage();
        LOGE(msg);
        throw FileSystemSetupException(msg);
    }
}

void touch(const std::string& path, mode_t mode)
{
    int fd = utils::open(path.c_str(), O_WRONLY | O_CREAT, mode);
    utils::close(fd);
}

void makeNode(const std::string& path, mode_t mode, dev_t dev)
{
    lxcpp::mknod(path, mode, dev);
    lxcpp::chmod(path, mode & 07777);
}

void pivotRoot(const std::string& new_root, const std::string& put_old)
{
    if (::syscall(SYS_pivot_root, new_root.c_str(), put_old.c_str()) < 0) {
        const std::string msg = "pivot_root() failed: " + utils::getSystemErrorMessage();
        LOGE(msg);
        throw FileSystemSetupException(msg);
    }
}

void containerChownRoot(const std::string& path, const struct UserNSConfig& config)
{
    uid_t rootUID = config.getContainerRootUID();
    gid_t rootGID = config.getContainerRootGID();

    lxcpp::chown(path, rootUID, rootGID);
}

} // namespace lxcpp
