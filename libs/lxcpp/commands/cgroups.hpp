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

#ifndef LXCPP_COMMANDS_CGROUPS_HPP
#define LXCPP_COMMANDS_CGROUPS_HPP

#include "lxcpp/commands/command.hpp"
#include "lxcpp/container-config.hpp"
#include "lxcpp/cgroups/cgroup-config.hpp"


namespace lxcpp {

class CGroupMakeAll final: Command {
public:
   /**
    * Creates and configures cgroups.
    */
    CGroupMakeAll(const CGroupsConfig& cfg, const UserNSConfig& uns) :
        mCgroups(cfg),
        mUserNS(uns)
    {
    }

    void execute();

private:
    const CGroupsConfig& mCgroups;
    const UserNSConfig& mUserNS;
};

class SubsystemMake final: Command {
public:
   /**
    * Creates and configures cgroup subsystem.
    */
    SubsystemMake(const SubsystemConfig& cfg) :
        mSubsys(cfg)
    {
    }

    void execute();

private:
    const SubsystemConfig& mSubsys;
};

class CGroupMake final: Command {
public:
   /**
    * Creates and configures cgroup.
    */
    CGroupMake(const CGroupConfig& cfg, const UserNSConfig& uns) :
        mCgroup(cfg),
        mUserNS(uns)
    {
    }

    void execute();

private:
    const CGroupConfig& mCgroup;
    const UserNSConfig& mUserNS;
};

class CGroupAssignPidAll final: Command {
public:
   /**
    * Assign pid to all configured cgroups
    */
    CGroupAssignPidAll(const CGroupsConfig& cfg, pid_t pid) :
        mCgroups(cfg),
        mPid(pid)
    {
    }

    void execute();

private:
    const CGroupsConfig& mCgroups;
    pid_t mPid;
};

class CGroupAssignPid final: Command {
public:
   /**
    * Assign pid to existing group.
    */
    CGroupAssignPid(const std::string& subsys, const std::string& cgroup, pid_t pid) :
        mSubsysName(subsys),
        mCgroupName(cgroup),
        mPid(pid)
    {
    }

    void execute();

private:
    std::string mSubsysName;
    std::string mCgroupName;
    pid_t mPid;
};

class PrepCGroupSysFs final: Command {
public:
   /**
    * Prepare cgroups sysfs hierarchy
    */
    PrepCGroupSysFs(const ContainerConfig& config) :
        mConfig(config)
    {
    }

    void execute();

private:
    const ContainerConfig& mConfig;
};

} // namespace lxcpp

#endif //LXCPP_COMMANDS_CGROUPS_HPP
