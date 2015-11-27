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
 * @brief   Headers of dev FS preparation command
 */

#ifndef LXCPP_COMMANDS_PREP_DEVFS_HPP
#define LXCPP_COMMANDS_PREP_DEVFS_HPP

#include "lxcpp/commands/command.hpp"
#include "lxcpp/container-config.hpp"


namespace lxcpp {


class PrepDevFS final: Command {
public:
    /**
     * Creates a virtual /dev and a private instance of /dev/pts to be used by the container
     *
     * It is necessary to do this outside of the container environment because
     * with user namespace the mknod(2) syscall is not permitted. Also because the devpts
     * filesystem has to be visible in both, the guard and the container. We use it to pass
     * console data between the container and the host. Guard uses both, container and host
     * PTYs file descriptors.
     *
     * @param config  A container config
     */
    PrepDevFS(ContainerConfig &config);
    ~PrepDevFS();

    void execute();
    void revert();

private:
    ContainerConfig &mConfig;
};


} // namespace lxcpp


#endif // LXCPP_COMMANDS_PREP_DEVFS_HPP
