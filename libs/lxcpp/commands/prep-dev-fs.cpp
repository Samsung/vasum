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
 * @brief   Implementation of dev FS preparation command
 */

#include "lxcpp/commands/prep-dev-fs.hpp"
#include "lxcpp/process.hpp"
#include "lxcpp/filesystem.hpp"

#include "logger/logger.hpp"
#include "utils/fs.hpp"
#include "utils/smack.hpp"
#include "utils/paths.hpp"

#include <sys/mount.h>

#define ARRAY_SIZE(a)  (sizeof(a) / sizeof(*(a)))

#define DEV_MAJOR_MEMORY  1
#define DEV_MINOR_NULL    3
#define DEV_MINOR_ZERO    5
#define DEV_MINOR_FULL    7
#define DEV_MINOR_RANDOM  8
#define DEV_MINOR_URANDOM 9

#define DEV_MAJOR_TTY     5
#define DEV_MINOR_TTY     0
#define DEV_MINOR_CONSOLE 1
#define DEV_MINOR_PTMX    2


namespace {

const struct dev {
    int major;
    int minor;
    mode_t type;
    mode_t mode;
    const char *path;
} static_devs[] = {
    { DEV_MAJOR_MEMORY, DEV_MINOR_NULL,    S_IFCHR, 0666, "/null" },
    { DEV_MAJOR_MEMORY, DEV_MINOR_ZERO,    S_IFCHR, 0666, "/zero" },
    { DEV_MAJOR_MEMORY, DEV_MINOR_FULL,    S_IFCHR, 0666, "/full" },
    { DEV_MAJOR_MEMORY, DEV_MINOR_RANDOM,  S_IFCHR, 0666, "/random" },
    { DEV_MAJOR_MEMORY, DEV_MINOR_URANDOM, S_IFCHR, 0666, "/urandom" },
    { DEV_MAJOR_TTY,    DEV_MINOR_TTY,     S_IFCHR, 0666, "/tty" },
};

} // namespace


namespace lxcpp {


PrepDevFS::PrepDevFS(ContainerConfig &config)
    : mConfig(config)
{
}

PrepDevFS::~PrepDevFS()
{
}

void PrepDevFS::execute()
{
    // Make sure the /dev/ and /dev/pts mounts we create below are invisible to the host
    lxcpp::unshare(CLONE_NEWNS);
    utils::mount("", "/", "", MS_SLAVE|MS_REC, "");

    // Future /dev
    const std::string devPath = utils::createFilePath(mConfig.mWorkPath,
                                                      mConfig.mName + ".dev");
    const std::string devOpts = "mode=755,size=65536";

    utils::mkdir(devPath, 0755);
    utils::mount("devfs", devPath, "tmpfs", MS_NOSUID, devOpts);
    containerChownRoot(devPath, mConfig.mUserNSConfig);

    for (unsigned i = 0; i < ARRAY_SIZE(static_devs); ++i) {
        dev_t dev = ::makedev(static_devs[i].major, static_devs[i].minor);
        std::string path = utils::createFilePath(devPath, static_devs[i].path);

        makeNode(path, static_devs[i].type | static_devs[i].mode, dev);
        containerChownRoot(path, mConfig.mUserNSConfig);
    }

    // Future /dev/pts
    const std::string devPtsPath = utils::createFilePath(mConfig.mWorkPath,
                                                         mConfig.mName + ".devpts");
    const std::string devPtsPtmx = utils::createFilePath(devPtsPath, "ptmx");

    // FIXME: A little bit hacky, root and tty GID can be disjoint.
    // A proper interface for recalculating namespaced UIDs/GIDs
    // should be provided in mUserNSConfig.
    const gid_t ptsGID = mConfig.mUserNSConfig.getContainerRootGID()
        + mConfig.mPtsGID;
    const std::string devPtsOpts = "newinstance,ptmxmode=0666,mode=0620,gid="
        + std::to_string(ptsGID);

    utils::mkdir(devPtsPath, 0755);
    utils::mount("devpts", devPtsPath, "devpts", MS_NOSUID, devPtsOpts);
    containerChownRoot(devPtsPath, mConfig.mUserNSConfig);
    containerChownRoot(devPtsPtmx, mConfig.mUserNSConfig);

    // Workaround for kernel bug/inconsistency. The root of the devfs mounted above
    // has floor label instead of the label of the process that mounted it.
    if (utils::isSmackActive()) {
        const std::string label = utils::smackGetSelfLabel();

        LOGD("Settings SMACK label of: " << devPath << " to: " << label);
        utils::smackSetFileLabel(devPath, label, utils::SmackLabelType::SMACK_LABEL_ACCESS, false);
        LOGD("Settings SMACK label of: " << devPtsPath << " to: " << label);
        utils::smackSetFileLabel(devPtsPath, label, utils::SmackLabelType::SMACK_LABEL_ACCESS, false);
    }
}

void PrepDevFS::revert()
{
    const std::string devPath = utils::createFilePath(mConfig.mWorkPath,
                                                      mConfig.mName + ".dev");
    utils::umount(devPath);

    const std::string devPtsPath = utils::createFilePath(mConfig.mWorkPath,
                                                         mConfig.mName + ".devpts");
    utils::umount(devPtsPath);
}

} // namespace lxcpp
