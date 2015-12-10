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

namespace lxcpp {

void CGroupMakeAll::execute()
{
    for (const auto i : mCgroups.subsystems) {
        SubsystemMake(i).execute();
    }

    for (const auto i : mCgroups.cgroups) {
        CGroupMake(i).execute();
    }
}

void SubsystemMake::execute()
{
    Subsystem sub(mSubsys.name);

    if (!sub.isAttached()) {
        sub.attach(mSubsys.path, {mSubsys.name});
    } else if (sub.getMountPoint() != mSubsys.path) {
        std::string msg = "Subsys " + mSubsys.name + " already mounted elsewhere";
        LOGW(msg);
        // no exception, usually subsystems are mounted
    }
}

void CGroupMake::execute()
{
    CGroup cgroup(mCgroup.subsystem, mCgroup.name);

    if (!cgroup.exists()) {
        cgroup.create();
    }

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

} // namespace lxcpp
