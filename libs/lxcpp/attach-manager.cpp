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

#include "lxcpp/attach-manager.hpp"
#include "lxcpp/exception.hpp"
#include "lxcpp/process.hpp"
#include "lxcpp/filesystem.hpp"
#include "lxcpp/namespace.hpp"
#include "lxcpp/capability.hpp"

#include "utils/exception.hpp"

#include <unistd.h>
#include <sys/mount.h>


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

} // namespace

AttachManager::AttachManager(lxcpp::ContainerImpl& container)
    : mContainer(container)
{
}

AttachManager::~AttachManager()
{
}

void AttachManager::attach(Container::AttachCall& call,
                           const std::string& wdInContainer)
{
    // Channels for setup synchronization
    utils::Channel intermChannel;

    const pid_t interPid = lxcpp::fork();
    if (interPid > 0) {
        intermChannel.setLeft();
        parent(intermChannel, interPid);
        intermChannel.shutdown();
    } else {
        intermChannel.setRight();
        interm(intermChannel, wdInContainer, call);
        intermChannel.shutdown();
        ::_exit(0);
    }
}

int AttachManager::child(void* data)
{
    try {
        // TODO Pass mask and options via data
        dropCapsFromBoundingExcept(0);
        setupMountPoints();
        return (*static_cast<Container::AttachCall*>(data))();
    } catch(...) {
        return -1; // Non-zero on failure
    }
    return 0; // Success
}

void AttachManager::parent(utils::Channel& intermChannel, const pid_t interPid)
{
    // TODO: Setup cgroups etc
    const pid_t childPid = intermChannel.read<pid_t>();

    // Wait for all processes
    lxcpp::waitpid(interPid);
    lxcpp::waitpid(childPid);
}

void AttachManager::interm(utils::Channel& intermChannel,
                           const std::string& wdInContainer,
                           Container::AttachCall& call)
{
    lxcpp::setns(mContainer.getInitPid(), mContainer.getNamespaces());

    // Change the current work directory
    // wdInContainer is a path relative to the container's root
    lxcpp::chdir(wdInContainer);

    // PID namespace won't affect the returned pid
    // CLONE_PARENT: Child's PPID == Caller's PID
    const pid_t childPid = lxcpp::clone(&AttachManager::child,
                                        &call,
                                        CLONE_PARENT);
    intermChannel.write(childPid);

}

} // namespace lxcpp
