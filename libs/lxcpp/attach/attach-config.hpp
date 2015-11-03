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
 * @author  Jan Olszak (j.olszak@samsung.com)
 * @brief   Internal structure sent between Attach command and AttachHelper
 */

#ifndef LXCPP_ATTACH_ATTACH_CONFIG_HPP
#define LXCPP_ATTACH_ATTACH_CONFIG_HPP

#include "lxcpp/namespace.hpp"

#include <config/config.hpp>
#include <config/fields.hpp>
#include "lxcpp/logger-config.hpp"
#include <sys/types.h>

#include <string>
#include <vector>

namespace lxcpp {

struct AttachConfig {

    /// Arguments passed by user, argv[0] is the binary's path in container
    std::vector<std::string> argv;

    /// PID of the container's init process
    pid_t initPid;

    /// Namespaces to which we'll attach
    int namespaces;

    /// User ID to set
    uid_t uid;

    /// Group ID to set
    gid_t gid;

    /// PTS that will become the control terminal for the attached process
    int ttyFD;

    /// Supplementary groups to set
    std::vector<gid_t> supplementaryGids;

    /// Mask of capabilities that will be available
    int capsToKeep;

    /// Work directory for the attached process
    std::string workDirInContainer;

    /// Environment variables that will be kept
    std::vector<std::string> envToKeep;

    /// Environment variables that will be set/updated for the attached process
    std::vector<std::pair<std::string, std::string>> envToSet;

    /// Logging on the attached process side
    LoggerConfig logger;

    AttachConfig() = default;

    AttachConfig(const std::vector<std::string>& argv,
                 const pid_t initPid,
                 const int namespaces,
                 const uid_t uid,
                 const gid_t gid,
                 const std::vector<gid_t>& supplementaryGids,
                 const int capsToKeep,
                 const std::string& workDirInContainer,
                 const std::vector<std::string>& envToKeep,
                 const std::vector<std::pair<std::string, std::string>>& envToSet,
                 const LoggerConfig & logger)
        : argv(argv),
          initPid(initPid),
          namespaces(namespaces),
          uid(uid),
          gid(gid),
          ttyFD(-1),
          supplementaryGids(supplementaryGids),
          capsToKeep(capsToKeep),
          workDirInContainer(workDirInContainer),
          envToKeep(envToKeep),
          envToSet(envToSet),
          logger(logger)
    {}

    CONFIG_REGISTER
    (
        argv,
        initPid,
        namespaces,
        uid,
        gid,
        ttyFD,
        supplementaryGids,
        capsToKeep,
        workDirInContainer,
        envToKeep,
        envToSet,
        logger
    )
};

} // namespace lxcpp

#endif // LXCPP_ATTACH_ATTACH_CONFIG_HPP
