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
 * @author  Krzysztof Dynowski (k.dynowski@samsumg.com)
 * @brief   CGroups configuration command
 */

#include "config.hpp"

#include "lxcpp/commands/cgroups.hpp"
#include "lxcpp/cgroups/cgroup.hpp"
#include "lxcpp/exception.hpp"
#include "logger/logger.hpp"

#include "utils/fs.hpp"
#include "utils/paths.hpp"

#include <sys/mount.h>

namespace lxcpp {

namespace {

const std::string& CGROUP_DEFAULT_PATH  = "/sys/fs/cgroup";
const std::string& INTERNAL_SYS_CGROUP = "internal";

}

void CGroupMakeAll::execute()
{
    for (const auto i : mCgroups.subsystems) {
        SubsystemMake(i).execute();
    }

    for (const auto i : mCgroups.cgroups) {
        CGroupMake(i, mUserNS).execute();
    }
}

void SubsystemMake::execute()
{
    Subsystem sub(mSubsys.name, mSubsys.path);

    if (!sub.isAttached()) {
        sub.attach(mSubsys.path, {mSubsys.name});
    }
}

void CGroupMake::execute()
{
    CGroup intCGroup(mCgroup.subsystem, mCgroup.name + "/" + INTERNAL_SYS_CGROUP);

    if (!intCGroup.exists()) {
        intCGroup.create();
    }

    if (mCgroup.subsystem == "systemd") {
        uid_t rootUID = mUserNS.convContToHostUID(0);
        gid_t rootGID = mUserNS.convContToHostGID(0);

        utils::chownDir(intCGroup.getPath(), rootUID, rootGID);
    }

    CGroup cgroup(mCgroup.subsystem, mCgroup.name);
    for (const auto& p : mCgroup.common) {
        cgroup.setCommonValue(p.name, p.value);
    }

    for (const auto& p : mCgroup.params) {
        cgroup.setValue(p.name, p.value);
    }
}

void CGroupAssignPidAll::execute()
{
    for (const auto& cgconf : mCgroups.cgroups) {
        CGroup cgroup(cgconf.subsystem, cgconf.name);
        cgroup.assignPid(mPid);
    }
}

void CGroupAssignPid::execute()
{
    CGroup cgroup(mSubsysName, mCgroupName);

    cgroup.assignPid(mPid);
}

void PrepCGroupSysFs::execute()
{
    utils::createDirs(CGROUP_DEFAULT_PATH, 0755);
    utils::mount("none", CGROUP_DEFAULT_PATH, "tmpfs", 0, "mode=755,size=65536");

    std::vector<std::string> subsystems;
    for (const auto& cgroup : mConfig.mCgroups.cgroups) {
        subsystems.push_back(cgroup.subsystem);
    }

    for (const auto &subsystem : subsystems) {
        Subsystem sub(subsystem, mConfig.mOldRoot);
        //TODO get "lxcpp" name from config or better from cgroups config
        const std::string& source = utils::createFilePath(sub.getMountPoint(), "lxcpp", mConfig.mName, INTERNAL_SYS_CGROUP);
        const std::string& target = utils::createFilePath(CGROUP_DEFAULT_PATH, sub.getName());

        utils::createDirs(target, 0755);
        utils::mount(source, target, "", MS_BIND, "");
    }
}

} // namespace lxcpp
