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

#ifndef LXCPP_CGROUPS_SUBSYSTEM_HPP
#define LXCPP_CGROUPS_SUBSYSTEM_HPP

#include <sys/types.h>

#include <string>
#include <vector>

class Subsystem {
public:
    /**
     * Define subsystem object
     */
    Subsystem(const std::string& name);

    /**
     * Check if named subsystem is supported by the kernel
     * @return true if subsystem is listed in /proc/cgroups
     */
    bool isAvailable();

    /**
     * Check if named subsystem is mounted (added to hierarchy)
     * @return true if subsystem has a mount point (as read from /proc/mounts)
     */
    bool isAttached();

    /**
     * Attach subsystem hierarchy to filesystem
     * Equivalent of: mount -t cgroup -o subs(coma-sep) subs(underln-sep) path
     */
    static void attach(const std::string& path, std::vector<std::string> subs);

    /**
     * Detach subsstem hierarchy from filesystem
     * Equivalent of: umount path
     */
    static void detach(const std::string& path);

    /**
     * Get list of available subsytems
     * @return parse contents of /proc/cgroups
     */
    static std::vector<std::string> availableSubsystems();

    /**
     * Get control groups list for a process (in format subsys_name:cgroup_name)
     * eg. "cpu:/user/test_user"
     * Equivalent of: cat /proc/pid/cgroup
     */
    static std::vector<std::string> getCGroups(pid_t pid);

private:
    const std::string& mName;
    std::string mPath;
};

#endif // LXCPP_CGROUPS_SUBSYSTEM_HPP
