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
#include "containers-manager.hpp"
#include "exception.hpp"

#include "config/manager.hpp"
#include "logger/logger.hpp"
#include "utils/glib-loop.hpp"
#include "utils/environment.hpp"
#include "utils/fs.hpp"

#include <csignal>
#include <cerrno>
#include <string>
#include <cstring>
#include <atomic>
#include <unistd.h>
#include <cap-ng.h>
#include <pwd.h>
#include <sys/stat.h>
#include <boost/filesystem.hpp>


#ifndef SECURITY_CONTAINERS_USER
#error "SECURITY_CONTAINERS_USER must be defined!"
#endif

#ifndef INPUT_EVENT_GROUP
#error "INPUT_EVENT_GROUP must be defined!"
#endif

#ifndef LIBVIRT_GROUP
#error "LIBVIRT_GROUP must be defined!"
#endif

#ifndef DISK_GROUP
#error "DISK_GROUP must be defined!"
#endif

#ifndef TTY_GROUP
#error "TTY_GROUP must be defined!"
#endif

extern char** environ;

namespace security_containers {


Server::Server(const std::string& configPath, bool runAsRoot)
    : mConfigPath(configPath)
{
    if (!prepareEnvironment(configPath, runAsRoot)) {
        throw ServerException("Environment setup failed");
    }
}


Server::~Server()
{
}


namespace {

std::atomic_bool gUpdateTriggered(false);
utils::Latch gSignalLatch;

void signalHandler(const int sig)
{
    LOGI("Got signal " << sig);

    if (sig == SIGUSR1) {
        LOGD("Received SIGUSR1 - triggering update.");
        gUpdateTriggered = true;
    }

    gSignalLatch.set();
}

} // namespace

void Server::run()
{
    signal(SIGINT,  signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGUSR1, signalHandler);

    LOGI("Starting daemon...");
    {
        utils::ScopedGlibLoop loop;
        ContainersManager manager(mConfigPath);

        manager.startAll();
        LOGI("Daemon started");

        gSignalLatch.wait();

        // Detach containers if we triggered an update
        if (gUpdateTriggered) {
            manager.setContainersDetachOnExit();
        }

        LOGI("Stopping daemon...");
        // manager.stopAll() will be called in destructor
    }
    LOGI("Daemon stopped");
}

void Server::reloadIfRequired(char* argv[])
{
    if (gUpdateTriggered) {
        execve(argv[0], argv, environ);

        LOGE("Failed to reload " << argv[0] << ": " << strerror(errno));
    }
}

void Server::terminate()
{
    LOGI("Terminating server");
    gSignalLatch.set();
}

bool Server::prepareEnvironment(const std::string& configPath, bool runAsRoot)
{
    namespace fs = boost::filesystem;

    // TODO: currently this config is loaded twice: here and in ContainerManager
    ContainersManagerConfig config;
    config::loadFromFile(configPath, config);

    struct passwd* pwd = ::getpwnam(SECURITY_CONTAINERS_USER);
    if (pwd == NULL) {
        LOGE("getpwnam failed to find user '" << SECURITY_CONTAINERS_USER << "'");
        return false;
    }
    uid_t uid = pwd->pw_uid;
    gid_t gid = pwd->pw_gid;
    LOGD("security-containers UID = " << uid << ", GID = " << gid);

    // create directory for dbus socket (if needed)
    if (!config.runMountPointPrefix.empty()) {
        if (!utils::createDir(config.runMountPointPrefix, uid, gid,
                              fs::perms::owner_all |
                              fs::perms::group_read | fs::perms::group_exe |
                              fs::perms::others_read | fs::perms::others_exe)) {
            return false;
        }
    }

    // create directory for additional container data (if needed)
    if (!config.containerNewConfigPrefix.empty()) {
        if (!utils::createDir(config.containerNewConfigPrefix, uid, gid,
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

    // LIBVIRT_GROUP provides access to libvirt's daemon socket.
    // INPUT_EVENT_GROUP provides access to /dev/input/event* devices used by InputMonitor.
    // DISK_GROUP provides access to /dev/loop* devices, needed when adding new container to copy
    //            containers image
    if (!utils::setSuppGroups({LIBVIRT_GROUP, INPUT_EVENT_GROUP, DISK_GROUP, TTY_GROUP})) {
        return false;
    }

    // CAP_SYS_ADMIN allows to mount tmpfs' for dbus communication at the runtime.
    // NOTE: CAP_MAC_OVERRIDE is temporary and must be removed when "smack namespace"
    // is introduced. The capability is needed to allow modify SMACK labels of
    // "/var/run/containers/<container>/run" mount point.
    // CAP_SYS_TTY_CONFIG is needed to activate virtual terminals through ioctl calls
    // CAP_CHOWN is needed when creating new container from image to set owner/group for each file,
    // directory or symlink
    // CAP_SETUID is needed to launch specific funtions as root (see environment.cpp)
    return (runAsRoot || utils::dropRoot(uid, gid, {CAP_SYS_ADMIN,
                                                    CAP_MAC_OVERRIDE,
                                                    CAP_SYS_TTY_CONFIG,
                                                    CAP_CHOWN,
                                                    CAP_SETUID}));
}


} // namespace security_containers
