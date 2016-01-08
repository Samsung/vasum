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

#include "lxcpp/container.hpp"
#include "lxcpp/logger-config.hpp"
#include "lxcpp/network-config.hpp"
#include "lxcpp/terminal-config.hpp"
#include "lxcpp/provision-config.hpp"
#include "lxcpp/userns-config.hpp"
#include "lxcpp/smackns-config.hpp"
#include "lxcpp/cgroups/cgroup-config.hpp"

#include <cargo/fields.hpp>

#include <string>
#include <vector>
#include <sys/types.h>
#include <map>

namespace {

const int DEFAULT_EXIT_STATUS = -27182;

} // namespace

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
     * Container hostname.
     * Set: by setHostName
     * Get: none
     */
    std::string mHostName;

    /**
     * Path of the root directory of the container.
     *
     * Set: by constructor, cannot be changed afterwards
     * Get: getRootPath()
     */
    std::string mRootPath;

    /**
     * Path of the work directory of the container.
     *
     * Set: by constructor, cannot be changed afterwards
     * Get: getWorkPath()
     */
    std::string mWorkPath;

    /**
     * Path of the old root after PivotRoot
     *
     * Set: by constructor, basically a const
     * Get: none, only for internal use
     */
    std::string mOldRoot;

    /**
     * Socket for communication with the Guard
     *
     * Set: by constructor, cannot be changed afterwards
     * Get: none
     */
    std::string mSocketPath;

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
     * State of the container
     *
     * Set: automatically on state transitions
     * Get: getState() and callbacks
     */
    Container::State mState;

    /**
     * Exit status of the stopped container
     *
     * Set: onGuardReady() onInitStopped() start() stop()
     * Get: getState() and callbacks
     */
    int mExitStatus;

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
     * Get: getNamespaces()
     */
    int mNamespaces;

    /**
     * available files/dirs/mounts/links
     *
     * Set: by container provision manipulation methods
     * Get: getFiles(), getMounts(), getLinks()
     */
    ProvisionConfig mProvisions;

    /**
     * User namespace config (uid and gid mappings)
     *
     * Set: addUIDMap(), addGIDMap()
     * Get: none
     */
    UserNSConfig mUserNSConfig;

    /**
     * Smack namespace config (mapping from original label to a new one).
     *
     * Set: addSmackLabelMap()
     * Get: none
     */
    SmackNSConfig mSmackNSConfig;

    /**
     * CGropus configuration
     */
    CGroupsConfig mCgroups;

    /**
     * Linux capabilities
     */
    unsigned long long mCapsToKeep;

    /**
     * Environment variables that will be set
     */
    std::vector<std::pair<std::string, std::string>> mEnvToSet;

    /**
     * Rlimits configuration
     * rlimit type, rlimit soft, rlimit hard
     */
    std::vector<std::tuple<int, uint64_t, uint64_t>> mRlimits;

    /**
     * Kernel parameters configuration
     */
    std::map<std::string, std::string> mKernelParameters;

    ContainerConfig():
        mOldRoot("/.oldroot"),
        mGuardPid(-1),
        mInitPid(-1),
        mState(Container::State::STOPPED),
        mExitStatus(DEFAULT_EXIT_STATUS),
        mNamespaces(0),
        mCapsToKeep(UINT64_MAX) {}

    CARGO_REGISTER
    (
        mName,
        mHostName,
        mRootPath,
        mWorkPath,
        mOldRoot,
        mGuardPid,
        mInitPid,
        mInit,
        mLogger,
        mState,
        mTerminals,
        mNetwork,
        mNamespaces,
        mProvisions,
        mUserNSConfig,
        mSmackNSConfig,
        mCgroups,
        mCapsToKeep,
        mEnvToSet,
        mRlimits,
        mKernelParameters
    )
};


}


#endif // LXCPP_CONTAINER_CONFIG_HPP
