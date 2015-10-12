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
 * @brief   A definition of a ContainerConfig struct
 */

#ifndef LXCPP_CONTAINER_CONFIG_HPP
#define LXCPP_CONTAINER_CONFIG_HPP

#include "lxcpp/logger-config.hpp"
#include "lxcpp/network-config.hpp"
#include "lxcpp/terminal-config.hpp"

#include <config/config.hpp>
#include <config/fields.hpp>

#include <string>
#include <vector>
#include <sys/types.h>


namespace lxcpp {


struct ContainerConfig {
    /**
     * Name of the container.
     *
     * Set: by constructor, cannot be changed afterwards.
     * Get: getName()
     */
    std::string mName;

    /**
     * Path of the root directory of the container.
     *
     * Set: by contstructor, cannot be changed afterwards.
     * Get: getRootPath()
     */
    std::string mRootPath;

    /**
     * Pid of the guard process.
     *
     * Set: automatically by the guard process itself.
     * Get: getGuardPid()
     */
    pid_t mGuardPid;

    /**
     * Pid of the container's init process.
     *
     * Set: automatically by the guard process.
     * Get: getInitPid()
     */
    pid_t mInitPid;

    /**
     * Container network configration
     *
     * Set: by container network config methods
     * Get: none
     */
    NetworkConfig mNetwork;

    /**
     * Argv of the container's init process to be executed.
     * The path has to be relative to the RootPath.
     *
     * Set: setInit()
     * Get: getInit()
     */
    std::vector<std::string> mInit;

    /**
     * Logger to be configured inside the guard process. This logger
     * reconfiguration is due to the fact that guard looses standard file
     * descriptors and might loose access to other files by mount namespace
     * usage. Hence an option to set some other logger that will work
     * regardless. E.g. PersistentFile.
     *
     * Set: setLogger()
     * Get: none
     */
    LoggerConfig mLogger;

    /**
     * Configuration for terminal(s), from API point of view, only their number.
     *
     * Set: setTerminalCount()
     * Get: none
     */
    TerminalsConfig mTerminals;

    /**
     * Namespace types used to create the container
     *
     * Set: setNamespaces()
     * Get: none
     */
    int mNamespaces;

    ContainerConfig() : mGuardPid(-1), mInitPid(-1), mNamespaces(0) {}

    CONFIG_REGISTER
    (
        mName,
        mRootPath,
        mGuardPid,
        mInitPid,
        mInit,
        mLogger,
        mTerminals,
        mNamespaces
    )
};


}


#endif // LXCPP_CONTAINER_CONFIG_HPP
