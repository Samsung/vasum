/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Jan Olszak (j.olszak@samsung.com)
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License
 */

/**
 * @file
 * @author  Jan Olszak (j.olszak@samsung.com)
 * @brief   Server class definition
 */

#include "config.hpp"

#include "server.hpp"
#include "exception.hpp"

#include "cargo-json/cargo-json.hpp"
#include "logger/logger.hpp"
#include "utils/environment.hpp"
#include "utils/fs.hpp"
#include "utils/signal.hpp"
#include "utils/exception.hpp"

#include <iostream>
#include <csignal>
#include <cerrno>
#include <string>
#include <functional>
#include <cstring>
#include <unistd.h>
#include <pwd.h>
#include <sys/stat.h>
#include <boost/filesystem.hpp>
#include <linux/capability.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/utsname.h>
#include <lxc/lxccontainer.h>
#include <lxc/version.h>


#ifndef VASUM_USER
#error "VASUM_USER must be defined!"
#endif

#ifndef INPUT_EVENT_GROUP
#error "INPUT_EVENT_GROUP must be defined!"
#endif

#ifndef DISK_GROUP
#error "DISK_GROUP must be defined!"
#endif

#ifndef TTY_GROUP
#error "TTY_GROUP must be defined!"
#endif

extern char** environ;

using namespace utils;

namespace vasum {

Server::Server(const std::string& configPath)
    : mIsRunning(true),
      mIsUpdate(false),
      mConfigPath(configPath),
      mSignalFD(mEventPoll),
      mZonesManager(mEventPoll, mConfigPath),
      mDispatchingThread(::pthread_self())
{
    mSignalFD.setHandler(SIGUSR1, std::bind(&Server::handleUpdate, this));
    mSignalFD.setHandler(SIGINT, std::bind(&Server::handleStop, this));
    mSignalFD.setHandler(SIGTERM, std::bind(&Server::handleStop, this));
}

void Server::handleUpdate()
{
    LOGD("Received SIGUSR1 - triggering update.");
    mZonesManager.setZonesDetachOnExit();
    mZonesManager.stop(false);
    mIsUpdate = true;
    mIsRunning = false;
}

void Server::handleStop()
{
    LOGD("Stopping Server");
    mZonesManager.stop(false);
    mIsRunning = false;
}

void Server::run(bool asRoot)
{
    if (!prepareEnvironment(mConfigPath, asRoot)) {
        throw ServerException("Environment setup failed");
    }

    mZonesManager.start();

    while(mIsRunning || mZonesManager.isRunning()) {
        mEventPoll.dispatchIteration(-1);
    }
}

void Server::reloadIfRequired(char* argv[])
{
    if (mIsUpdate) {
        ::execve(argv[0], argv, environ);
        LOGE("Failed to reload " << argv[0] << ": " << getSystemErrorMessage());
    }
}

void Server::terminate()
{
    LOGI("Terminating server");
    int ret = ::pthread_kill(mDispatchingThread, SIGINT);
    if(ret != 0) {
        const std::string msg = "Error during Server termination: " + utils::getSystemErrorMessage(ret);
        LOGE(msg);
        throw ServerException(msg);
    }
}

bool Server::checkEnvironment()
{
    // check kernel
    struct utsname u;
    int version, major, minor;
    ::uname(&u);
    version = major = minor = 0;
    ::sscanf(u.release, "%d.%d.%d", &version, &major, &minor);
    if (version < 2 || (version == 2 && major < 6) || (version == 2 && major == 6 && minor < 29)) {
        // control-group functionality was merged into kernel version 2.6.24 in 2007 (wikipedia)
        // namespace support begins from kernels 2.4.19(mnt), 2.6.19(ns,uts,ipc), 2.6.24(pid), 2.6.29(net)
        // namespace for usr from kernel 3.8(usr) - not used by vasum
        std::cout << "kernel is old ver=" << u.release << ", run vasum-check-config" << std::endl;
        return false;
    }
    else
        std::cout << "kernel " << u.release << " [OK]" << std::endl;

    // check lxc (TODO check if running on broken ABI version)
    if (::strcmp(lxc_get_version(), LXC_VERSION)!=0) {
        // versions that matters:
        // 1.1.0 added function ptr 'in-the-middle' destroy_with_snapshots, snapshot_destroy_all (breaks ABI)
        // 1.1.2 added function ptr 'append' attach_interface,detach_interface,checkpoint,restore (safe for ABI)
        std::cout << "LXC version not match, compiled for " << LXC_VERSION << ", installed " << lxc_get_version() << std::endl;
        return false;
    }
    else
        std::cout << "LXC version " << lxc_get_version() << " [OK]" << std::endl;

    // check cgroups (and its subsystems?)
    std::string cgroupCheck = "/sys/fs/cgroup";
    int fd = ::open(cgroupCheck.c_str(), O_RDONLY);
    if (fd == -1) {
        std::cout << "no cgroups support (can't access " << cgroupCheck << "), run vasum-check-config" << std::endl;
        return false;
    }

    bool err = false;
    std::vector<std::string> cgroupSubsCheck = {"cpu", "cpuset", "memory"};
    for (std::string f : cgroupSubsCheck)  {
        if (::faccessat(fd, f.c_str(), R_OK|X_OK, 0) == -1) {
            std::cout << "no cgroups support (can't access " << cgroupCheck << "/" << f << ")" << std::endl;
            err=true;
        }
    }
    ::close(fd);
    if (err) {
        std::cout << "cgroups problem, run vasum-check-config" << std::endl;
        return false;
    }
    else
        std::cout << "cgroups support " << " [OK]" << std::endl;

    // check namespaces
    std::string nsCheck = "/proc/self/ns";
    if (::access(nsCheck.c_str(), R_OK|X_OK)  == -1) {
        std::cout << "no namespace support (can't access " << nsCheck << "), run vasum-check-config" << std::endl;
        return false;
    }
    else
        std::cout << "namespaces support " << " [OK]" << std::endl;
    ::close(fd);

    return true;
}


bool Server::prepareEnvironment(const std::string& configPath, bool runAsRoot)
{
    namespace fs = boost::filesystem;

    // TODO: currently this config is loaded twice: here and in ZonesManager
    ZonesManagerConfig config;
    cargo::loadFromJsonFile(configPath, config);

    struct passwd* pwd = ::getpwnam(VASUM_USER);
    if (pwd == NULL) {
        LOGE("getpwnam failed to find user '" << VASUM_USER << "'");
        return false;
    }
    uid_t uid = pwd->pw_uid;
    gid_t gid = pwd->pw_gid;
    LOGD("vasum UID = " << uid << ", GID = " << gid);

    // create directory for dbus socket (if needed)
    if (!config.runMountPointPrefix.empty()) {
        if (!utils::createDir(config.runMountPointPrefix, uid, gid,
                              fs::perms::owner_all |
                              fs::perms::group_read | fs::perms::group_exe |
                              fs::perms::others_read | fs::perms::others_exe)) {
            return false;
        }
    }

    // Omit supplementaty group setup and root drop if the user is already switched.
    // This situation will happen during daemon update triggered by SIGUSR1.
    if (!runAsRoot && geteuid() == uid) {
        return true;
    }

    // INPUT_EVENT_GROUP provides access to /dev/input/event* devices used by InputMonitor.
    // DISK_GROUP provides access to /dev/loop* devices, needed when adding new zone to copy
    //            zones image
    if (!utils::setSuppGroups({INPUT_EVENT_GROUP, DISK_GROUP, TTY_GROUP})) {
        return false;
    }

    // CAP_SYS_ADMIN allows to mount tmpfs' for dbus communication at the runtime.
    // NOTE: CAP_MAC_OVERRIDE is temporary and must be removed when "smack namespace"
    // is introduced. The capability is needed to allow modify SMACK labels of
    // "/var/run/zones/<zone>/run" mount point.
    // CAP_SYS_TTY_CONFIG is needed to activate virtual terminals through ioctl calls
    // CAP_CHOWN is needed when creating new zone from image to set owner/group for each file,
    // directory or symlink
    // CAP_SETUID is needed to launch specific funtions as root (see environment.cpp)
    return (runAsRoot || utils::dropRoot(uid, gid, {CAP_SYS_ADMIN,
                                         CAP_MAC_OVERRIDE,
                                         CAP_SYS_TTY_CONFIG,
                                         CAP_CHOWN,
                                         CAP_SETUID
                                                   }));
}


} // namespace vasum
