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
 * @author  Mateusz Malicki (m.malicki2@samsung.com)
 * @brief   ContainerImpl definition
 */

#include "lxcpp/container-impl.hpp"
#include "lxcpp/exception.hpp"
#include "lxcpp/process.hpp"
#include "lxcpp/filesystem.hpp"
#include "lxcpp/namespace.hpp"
#include "lxcpp/capability.hpp"

#include "utils/exception.hpp"

#include <unistd.h>
#include <sys/mount.h>


namespace lxcpp {

ContainerImpl::ContainerImpl()
{
}

ContainerImpl::~ContainerImpl()
{
}

std::string ContainerImpl::getName()
{
    throw NotImplementedException();
}

void ContainerImpl::setName(const std::string& /* name */)
{
    throw NotImplementedException();
}

void ContainerImpl::start()
{
    throw NotImplementedException();
}

void ContainerImpl::stop()
{
    throw NotImplementedException();
}

void ContainerImpl::freeze()
{
    throw NotImplementedException();
}

void ContainerImpl::unfreeze()
{
    throw NotImplementedException();
}

void ContainerImpl::reboot()
{
    throw NotImplementedException();
}

int ContainerImpl::getInitPid()
{
    throw NotImplementedException();
}

void ContainerImpl::create()
{
    throw NotImplementedException();
}

void ContainerImpl::destroy()
{
    throw NotImplementedException();
}

void ContainerImpl::setRootPath(const std::string& /* path */)
{
    throw NotImplementedException();
}

std::string ContainerImpl::getRootPath()
{
    throw NotImplementedException();
}

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

int ContainerImpl::attachChild(void* data) {
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

void ContainerImpl::attachParent(utils::Channel& channel, const pid_t interPid)
{
    // TODO: Setup cgroups etc
    pid_t childPid = channel.read<pid_t>();

    // Wait for the Intermediate process
    lxcpp::waitpid(interPid);

    // Wait for the Child process
    lxcpp::waitpid(childPid);
}

void ContainerImpl::attachIntermediate(utils::Channel& channel, Container::AttachCall& call)
{
    lxcpp::setns(mInitPid, mNamespaces);

    // PID namespace won't affect the returned pid
    // CLONE_PARENT: Child's PPID == Caller's PID
    const pid_t pid = lxcpp::clone(&ContainerImpl::attachChild,
                                   &call,
                                   CLONE_PARENT);
    channel.write(pid);
}

void ContainerImpl::attach(Container::AttachCall& call)
{
    utils::Channel channel;

    const pid_t interPid = lxcpp::fork();
    if (interPid > 0) {
        channel.setLeft();
        attachParent(channel, interPid);
        channel.shutdown();
    } else {
        channel.setRight();
        attachIntermediate(channel, call);
        channel.shutdown();
        ::_exit(0);
    }
}



} // namespace lxcpp
