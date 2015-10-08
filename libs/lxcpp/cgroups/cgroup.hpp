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
 * @brief   Control-groups management
 */

#ifndef LXCPP_CGROUPS_CGROUP_HPP
#define LXCPP_CGROUPS_CGROUP_HPP

#include "lxcpp/cgroups/subsystem.hpp"

class CGroup {

public:
    /**
     * Define control-group object
     */
    CGroup(const Subsystem& subsys, const std::string& name) :
        mSubsys(subsys),
        mName(name)
    {
    }

    /**
     * Check if cgroup exists
     * @return true if cgroup path (subsys.path / mName) exists
     */
    bool exists();

    /**
     * Create cgroup directory
     * Equivalent of: mkdir subsys.path / mName
     */
    void create();

    /**
     * Destroy cgroup directory
     * Equivalent of: rmdir subsys.path / mName
     * Note: set memory.force_empty before removing a cgroup to avoid moving out-of-use page caches to parent
     */
    void destroy();

    /**
     * Set cgroup parameter to value (name validity depends on subsystem)
     * Equivalent of: echo value > mSubsys_path/mName/mSubsys_name.param
     */
    void setValue(const std::string& param, const std::string& value);

    /**
     * Get cgroup parameter
     * Equivalent of: cat mSubsys_path/mName/mSubsys_name.param
     */
    std::string getValue(const std::string& key);

    /**
     * Move process to this cgroup (process can't be removed from a cgroup)
     * Equivalent of: echo pid > mSubsys_path/mName/tasks
     */
    void moveProcess(pid_t pid);

private:
    const Subsystem& mSubsys; // referred subsystem
    const std::string& mName; // path relative to subsystem "root"
};

#endif // LXCPP_CGROUPS_CGROUP_HPP
