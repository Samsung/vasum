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
 * @brief   Headers of root FS preparation command
 */

#ifndef LXCPP_COMMANDS_PIVOT_AND_PREP_ROOT_HPP
#define LXCPP_COMMANDS_PIVOT_AND_PREP_ROOT_HPP

#include "lxcpp/commands/command.hpp"
#include "lxcpp/container-config.hpp"


namespace lxcpp {


class PivotAndPrepRoot final: Command {
public:
    /**
     * Does a pivot_root syscall and prepares the resulting root fs for its usage
     *
     * After the pivot_root previously prepared /dev and /dev/pts filesystems are mounted,
     * as well as a list of static mounts that are required for a fully working OS.
     * Things like /proc, /sys and security filesystem (if permitted).
     *
     * @param config  A container config
     */
    PivotAndPrepRoot(ContainerConfig &config);
    ~PivotAndPrepRoot();

    void execute();

private:
    ContainerConfig &mConfig;
    const bool mIsUserNamespace;
    const bool mIsNetNamespace;

    void pivotRoot();
    void cleanUpRoot();
    void mountStatic();
    void prepDev();
    void symlinkStatic();
};


} // namespace lxcpp


#endif // LXCPP_COMMANDS_PIVOT_AND_PREP_ROOT_HPP
