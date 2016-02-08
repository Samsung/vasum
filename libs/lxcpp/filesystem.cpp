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
#include "utils/fs.hpp"
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

namespace {

const int MNT_BUF_SIZE = 1024;

} // namespace


namespace lxcpp {

void bindMountFile(const std::string &source, const std::string &target)
{
    LOGD("Bind mounting: " << source << " to: " << target);

    utils::createFile(target, 0, 0666);
    utils::mount(source, target, "", MS_BIND, "");
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
            utils::umount(mnt);
        } catch(...) {
            LOGD("Failed to umount: " << mnt << " trying to detach: " << *mounts.rbegin());
            utils::umount(*mounts.rbegin(), MNT_DETACH);
            utils::umount(*mounts.rbegin());
            break;
        }
    }
}

void makeNode(const std::string& path, mode_t mode, dev_t dev)
{
    utils::mknod(path, mode, dev);
    utils::chmod(path, mode & 07777);
}

void containerChownRoot(const std::string& path, const struct UserNSConfig& config)
{
    uid_t rootUID = config.getContainerRootUID();
    gid_t rootGID = config.getContainerRootGID();

    utils::chown(path, rootUID, rootGID);
}

} // namespace lxcpp
