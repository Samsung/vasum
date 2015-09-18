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
 * @brief   Implementation of attaching to a container
 */

#include "lxcpp/commands/attach.hpp"
#include "lxcpp/exception.hpp"
#include "lxcpp/process.hpp"
#include "lxcpp/filesystem.hpp"
#include "lxcpp/namespace.hpp"
#include "lxcpp/capability.hpp"
#include "lxcpp/environment.hpp"
#include "lxcpp/credentials.hpp"

#include "utils/exception.hpp"
#include "utils/fd-utils.hpp"
#include "logger/logger.hpp"

#include <unistd.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <fcntl.h>

#include <functional>

namespace lxcpp {

namespace {

void setupMountPoints()
{
    /* TODO: Uncomment when preparing the final attach() version

    // TODO: This unshare should be optional only if we attach to PID/NET namespace, but not MNT.
    // Otherwise container already has remounted /proc /sys
    lxcpp::unshare(Namespace::MNT);

    if (isMountPointShared("/")) {
        // TODO: Handle case when the container rootfs or mount location is MS_SHARED, but not '/'
        lxcpp::mount(nullptr, "/", nullptr, MS_SLAVE | MS_REC, nullptr);
    }

    if(isMountPoint("/proc")) {
        lxcpp::umount("/proc", MNT_DETACH);
        lxcpp::mount("none", "/proc", "proc", 0, nullptr);
    }

    if(isMountPoint("/sys")) {
        lxcpp::umount("/sys", MNT_DETACH);
        lxcpp::mount("none", "/sys", "sysfs", 0, nullptr);
    }

    */
}

bool setupControlTTY(const int ttyFD)
{
    if (!::isatty(ttyFD)) {
        return false;
    }

    if (::setsid() < 0) {
        return false;
    }

    if (::ioctl(ttyFD, TIOCSCTTY, NULL) < 0) {
        return false;
    }

    if (::dup2(ttyFD, STDIN_FILENO) < 0) {
        return false;
    }

    if (::dup2(ttyFD, STDOUT_FILENO) < 0) {
        return false;
    }

    if (::dup2(ttyFD, STDERR_FILENO) < 0) {
        return false;
    }

    return true;
}

int execFunction(void* call)
{
    try {
        return (*static_cast<Attach::Call*>(call))();
    } catch(...) {
        return -1; // Non-zero on failure
    }
    return 0; // Success
}

} // namespace

Attach::Attach(lxcpp::ContainerImpl& container,
               Container::AttachCall& userCall,
               const uid_t uid,
               const gid_t gid,
               const std::string& ttyPath,
               const std::vector<gid_t>& supplementaryGids,
               const int capsToKeep,
               const std::string& workDirInContainer,
               const std::vector<std::string>& envToKeep,
               const std::vector<std::pair<std::string, std::string>>& envToSet)
    : mContainer(container),
      mUserCall(userCall),
      mUid(uid),
      mGid(gid),
      mSupplementaryGids(supplementaryGids),
      mCapsToKeep(capsToKeep),
      mWorkDirInContainer(workDirInContainer),
      mEnvToKeep(envToKeep),
      mEnvToSet(envToSet)
{
    mTTYFD = utils::open(ttyPath, O_RDWR | O_NOCTTY);
}

Attach::~Attach()
{
    utils::close(mTTYFD);
}

void Attach::execute()
{
    // Channels for setup synchronization
    utils::Channel intermChannel;

    Call call = std::bind(&Attach::child,
                          mUserCall,
                          mUid,
                          mGid,
                          mTTYFD,
                          mSupplementaryGids,
                          mCapsToKeep,
                          mEnvToKeep,
                          mEnvToSet);

    const pid_t interPid = lxcpp::fork();
    if (interPid > 0) {
        intermChannel.setLeft();
        parent(intermChannel, interPid);
        intermChannel.shutdown();
    } else {
        // TODO: only safe functions mentioned in signal(7) should be used below.
        // This is not the case now, needs fixing.
        intermChannel.setRight();
        interm(intermChannel, call);
        intermChannel.shutdown();
        ::_exit(EXIT_SUCCESS);
    }
}

int Attach::child(const Container::AttachCall& call,
                  const uid_t uid,
                  const gid_t gid,
                  const int ttyFD,
                  const std::vector<gid_t>& supplementaryGids,
                  const int capsToKeep,
                  const std::vector<std::string>& envToKeep,
                  const std::vector<std::pair<std::string, std::string>>& envToSet)
{
    // Setup /proc /sys mount
    setupMountPoints();

    // Setup capabilities
    dropCapsFromBoundingExcept(capsToKeep);

    // Setup environment variables
    clearenvExcept(envToKeep);
    setenv(envToSet);

    // Set uid/gids
    lxcpp::setgid(gid);
    setgroups(supplementaryGids);

    lxcpp::setuid(uid);

    // Set control TTY
    if(!setupControlTTY(ttyFD)) {
        ::_exit(EXIT_FAILURE);
    }

    // Run user's code
    return call();
}

void Attach::parent(utils::Channel& intermChannel, const pid_t interPid)
{
    // TODO: Setup cgroups etc
    const pid_t childPid = intermChannel.read<pid_t>();

    // Wait for all processes
    lxcpp::waitpid(interPid);
    lxcpp::waitpid(childPid);
}

void Attach::interm(utils::Channel& intermChannel, Call& call)
{
    lxcpp::setns(mContainer.getInitPid(), mContainer.getNamespaces());

    // Change the current work directory
    // workDirInContainer is a path relative to the container's root
    lxcpp::chdir(mWorkDirInContainer);

    // PID namespace won't affect the returned pid
    // CLONE_PARENT: Child's PPID == Caller's PID
    const pid_t childPid = lxcpp::clone(execFunction,
                                        &call,
                                        CLONE_PARENT);
    intermChannel.write(childPid);
}

} // namespace lxcpp
