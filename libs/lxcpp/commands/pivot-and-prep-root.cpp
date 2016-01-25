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
 * @author  Lukasz Pawelczyk (l.pawelczyk@samsung.com)
 * @brief   Implementation of root FS preparation command
 */

#include "lxcpp/commands/pivot-and-prep-root.hpp"
#include "lxcpp/filesystem.hpp"
#include "lxcpp/smack.hpp"

#include "logger/logger.hpp"
#include "utils/paths.hpp"

#include <sys/mount.h>

#define ARRAY_SIZE(a)  (sizeof(a) / sizeof(*(a)))


namespace {

const std::string SELINUX_MOUNT_PATH = "/sys/fs/selinux";

const struct mount {
    const char *src;
    const char *dst;
    const char *type;
    int flags;
    bool skipUserNS;
    bool skipUnmounted;
    bool skipNoNetNS;
} staticMounts[] = {
    { "proc", "/proc", "proc", MS_NOSUID|MS_NOEXEC|MS_NODEV, false, false, false },
    { "/proc/sys", "/proc/sys", "", MS_BIND|MS_NOSUID|MS_NOEXEC|MS_NODEV|MS_RDONLY, false, false, false },
    { "sysfs", "/sys", "sysfs", MS_NOSUID|MS_NOEXEC|MS_NODEV|MS_RDONLY, false, false, true },
    { "securityfs", "/sys/kernel/security", "securityfs", MS_NOSUID|MS_NOEXEC|MS_NODEV|MS_RDONLY, true, true, false },
    { "selinuxfs", SELINUX_MOUNT_PATH.c_str(), "selinuxfs", MS_NOSUID|MS_NOEXEC|MS_NODEV|MS_RDONLY, true, true, false },
    { "smackfs", SMACK_MOUNT_PATH, "smackfs", MS_NOSUID|MS_NOEXEC|MS_NODEV|MS_RDONLY, true, true, false },
};

const struct link {
    const char *src;
    const char *dst;
} staticLinks[] = {
    { "/proc/self/fd/0", "/dev/stdin" },
    { "/proc/self/fd/1", "/dev/stdout" },
    { "/proc/self/fd/2", "/dev/stderr" },
    { "/proc/self/fd", "/dev/fd" },
};

} // namespace


namespace lxcpp {


PivotAndPrepRoot::PivotAndPrepRoot(ContainerConfig &config)
    : mConfig(config),
      mIsUserNamespace(mConfig.mNamespaces & CLONE_NEWUSER),
      mIsNetNamespace(mConfig.mNamespaces & CLONE_NEWNET)
{
}

PivotAndPrepRoot::~PivotAndPrepRoot()
{
}

void PivotAndPrepRoot::execute()
{
    pivotRoot();
    cleanUpRoot();
    mountStatic();
    prepDev();
    symlinkStatic();
}

void PivotAndPrepRoot::pivotRoot()
{
    lxcpp::mount("", "/", "", MS_PRIVATE|MS_REC, "");

    const std::string oldRootPath = utils::createFilePath(mConfig.mRootPath, mConfig.mOldRoot);
    const std::string newRootPath = utils::createFilePath(oldRootPath, "/newroot");

    // Create a tmpfs and a directory for the new root as it has
    // to be on a separate mount point than the current one.

    lxcpp::mkdir(oldRootPath, 0755);
    lxcpp::mount("tmprootfs", oldRootPath, "tmpfs", 0, "");

    lxcpp::mkdir(newRootPath, 0755);
    lxcpp::mount(mConfig.mRootPath, newRootPath, "", MS_BIND|MS_REC, "");

    lxcpp::chdir(newRootPath);
    lxcpp::pivotRoot(".", "." + mConfig.mOldRoot);
    lxcpp::chdir("/");
}

void PivotAndPrepRoot::cleanUpRoot()
{
    // With mRootPath == "/" and user namespace the code below (umountSubtree() specifically)
    // will fail with ENOPERM as it should. Using "/" with user namespace is not supported.

    if (mConfig.mRootPath.compare("/") != 0) {
        return;
    }

    // Clean up the remounted "/" so it's ready to be reused.
    LOGD("Reusing '/' filesystem, umounting everything first");

    const std::string devPrepared = utils::createFilePath(mConfig.mWorkPath,
                                                          mConfig.mName + ".dev");

    lxcpp::umount(devPrepared);

    umountSubtree("/sys");
    umountSubtree("/dev");
    umountSubtree("/proc");
}

void PivotAndPrepRoot::mountStatic()
{
    for (unsigned i = 0; i < ARRAY_SIZE(staticMounts); ++i) {
        const struct mount &m = staticMounts[i];

        if (m.skipUserNS && mIsUserNamespace) {
            LOGD("Not mounting " << m.dst << " it's marked to be skipped with user namespace");
            continue;
        }

        if (m.skipNoNetNS && !mIsNetNamespace && mIsUserNamespace) {
            LOGD("Not mounting " << m.dst << " it's marked to be skipped without net namespace");
            continue;
        }

        if (m.skipUnmounted) {
            const std::string hostPath = mConfig.mOldRoot + std::string(m.dst);
            bool existsDir = lxcpp::exists(hostPath, S_IFDIR);

            if (!existsDir || !lxcpp::isMountPoint(hostPath)) {
                LOGD("Not mounting " << m.dst << " it's not mounted on the host");
                continue;
            }
        }

        LOGD("Mounting: " << m.src << " on: " << m.dst << " type: " << m.type);
        lxcpp::mkdir(m.dst, 0755);
        lxcpp::mount(m.src, m.dst, m.type, m.flags, "");
    }
}

void PivotAndPrepRoot::prepDev()
{
    int devFlags = mIsUserNamespace ? MS_BIND : MS_MOVE;

    // Use previously prepared dev as new /dev
    const std::string devPrepared = utils::createFilePath(mConfig.mOldRoot,
                                                          mConfig.mWorkPath,
                                                          mConfig.mName + ".dev");

    lxcpp::mkdir("/dev", 0755);
    lxcpp::mount(devPrepared, "/dev", "", devFlags, "");

    // Use previously prepared devpts as new /dev/pts
    const std::string devPtsPrepared = utils::createFilePath(mConfig.mOldRoot,
                                                             mConfig.mWorkPath,
                                                             mConfig.mName + ".devpts");
    lxcpp::mkdir("/dev/pts", 0755);
    lxcpp::mount(devPtsPrepared, "/dev/pts", "", devFlags, "");
}

void PivotAndPrepRoot::symlinkStatic()
{
    for (unsigned i = 0; i < ARRAY_SIZE(staticLinks); ++i) {
        LOGD("Symlinking: " << staticLinks[i].src << " to: " << staticLinks[i].dst);
        lxcpp::symlink(staticLinks[i].src, staticLinks[i].dst);
    }
}


} // namespace lxcpp
