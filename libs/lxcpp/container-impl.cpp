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
#include "lxcpp/capability.hpp"
#include "lxcpp/commands/attach.hpp"
#include "lxcpp/commands/console.hpp"
#include "lxcpp/commands/start.hpp"
#include "lxcpp/commands/stop.hpp"
#include "lxcpp/commands/prep-host-terminal.hpp"
#include "lxcpp/commands/provision.hpp"

#include "logger/logger.hpp"
#include "utils/exception.hpp"

#include <unistd.h>
#include <sys/mount.h>
#include <sys/wait.h>
#include <mutex>
#include <algorithm>

#include <ext/stdio_filebuf.h>
#include <fstream>
#include <iostream>
#include <stdio.h>


namespace lxcpp {

ContainerImpl::ContainerImpl(const std::string &name, const std::string &path)
{
    if (name.empty()) {
        const std::string msg = "Name cannot be empty";
        LOGE(msg);
        throw ConfigureException(msg);
    }

    if (path.empty()) {
        const std::string msg = "Path cannot be empty";
        LOGE(msg);
        throw ConfigureException(msg);
    }

    if(::access(path.c_str(), X_OK) < 0) {
        const std::string msg = "Path must point to a traversable directory";
        LOGE(msg);
        throw ConfigureException(msg);
    }

    mConfig.mName = name;
    mConfig.mRootPath = path;
    mConfig.mNamespaces = CLONE_NEWIPC | CLONE_NEWNS | CLONE_NEWPID | CLONE_NEWUTS;
}

// TODO: the aim of this constructor is to create a new ContainerImpl based on an already
// running container. It should talk to its guard and receive its current config.
ContainerImpl::ContainerImpl(pid_t /*guardPid*/)
{
    throw NotImplementedException();
}

ContainerImpl::~ContainerImpl()
{
}

const std::string& ContainerImpl::getName() const
{
    return mConfig.mName;
}

const std::string& ContainerImpl::getRootPath() const
{
    return mConfig.mRootPath;
}

const std::vector<std::string>& ContainerImpl::getInit()
{
    return mConfig.mInit;
}

void ContainerImpl::setInit(const std::vector<std::string> &init)
{
    if (init.empty() || init[0].empty()) {
        const std::string msg = "Init path cannot be empty";
        LOGE(msg);
        throw ConfigureException(msg);
    }

    std::string path = mConfig.mRootPath + "/" + init[0];

    if (::access(path.c_str(), X_OK) < 0) {
        const std::string msg = "Init path must point to an executable file";
        LOGE(msg);
        throw ConfigureException(msg);
    }

    mConfig.mInit = init;
}

pid_t ContainerImpl::getGuardPid() const
{
    return mConfig.mGuardPid;
}

pid_t ContainerImpl::getInitPid() const
{
    return mConfig.mInitPid;
}

void ContainerImpl::setLogger(const logger::LogType type,
                              const logger::LogLevel level,
                              const std::string &arg)
{
    mConfig.mLogger.set(type, level, arg);
}

void ContainerImpl::setTerminalCount(const unsigned int count)
{
    if (count == 0) {
        const std::string msg = "Container needs at least one terminal";
        LOGE(msg);
        throw ConfigureException(msg);
    }

    mConfig.mTerminals.count = count;
}

void ContainerImpl::addUIDMap(unsigned min, unsigned max, unsigned num)
{
    mConfig.mNamespaces |= CLONE_NEWUSER;

    if (mConfig.mUserNSConfig.mUIDMaps.size() >= 5) {
        const std::string msg = "Max number of 5 UID mappings has been already reached";
        LOGE(msg);
        throw ConfigureException(msg);
    }

    mConfig.mUserNSConfig.mUIDMaps.emplace_back(min, max, num);
}

void ContainerImpl::addGIDMap(unsigned min, unsigned max, unsigned num)
{
    mConfig.mNamespaces |= CLONE_NEWUSER;

    if (mConfig.mUserNSConfig.mGIDMaps.size() >= 5) {
        const std::string msg = "Max number of 5 GID mappings has been already reached";
        LOGE(msg);
        throw ConfigureException(msg);
    }

    mConfig.mUserNSConfig.mGIDMaps.emplace_back(min, max, num);
}

void ContainerImpl::start()
{
    // TODO: check config consistency and completeness somehow

    PrepHostTerminal terminal(mConfig.mTerminals);
    terminal.execute();

    Start start(mConfig);
    start.execute();
}

void ContainerImpl::stop()
{
    // TODO: things to do when shutting down the container:
    // - close PTY master FDs from the config so we won't keep PTYs open

    Stop stop(mConfig);
    stop.execute();
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

void ContainerImpl::attach(const std::vector<std::string>& argv,
                           const std::string& cwdInContainer)
{
    Attach attach(mConfig,
                  argv,
                  /*uid in container*/ 0,
                  /*gid in container*/ 0,
                  "/dev/tty",
                  /*supplementary gids in container*/ {},
                  /*capsToKeep*/ 0,
                  cwdInContainer,
                  /*envToKeep*/ {},
                  /*envInContainer*/ {{"container","lxcpp"}},
                  mConfig.mLogger);
    // TODO: Env variables should agree with the ones already in the container
    attach.execute();
}

void ContainerImpl::console()
{
    Console console(mConfig.mTerminals);
    console.execute();
}

bool ContainerImpl::isRunning() const
{
    // TODO: race condition may occur, sync needed
    return getInitPid() != -1;
}

void ContainerImpl::addInterfaceConfig(const std::string& hostif,
                                       const std::string& zoneif,
                                       InterfaceType type,
                                       const std::vector<InetAddr>& addrs,
                                       MacVLanMode mode)
{
    mConfig.mNamespaces |= CLONE_NEWNET;
    mConfig.mNetwork.addInterfaceConfig(hostif, zoneif, type, addrs, mode);
}

void ContainerImpl::addInetConfig(const std::string& ifname, const InetAddr& addr)
{
    mConfig.mNetwork.addInetConfig(ifname, addr);
}

std::vector<std::string> ContainerImpl::getInterfaces() const
{
    return NetworkInterface::getInterfaces(getInitPid());
}

NetworkInterfaceInfo ContainerImpl::getInterfaceInfo(const std::string& ifname) const
{
    NetworkInterface ni(ifname, getInitPid());
    std::vector<InetAddr> addrs;
    std::string macaddr;
    int mtu = 0, flags = 0;
    const Attrs& attrs = ni.getAttrs();
    for (const Attr& a : attrs) {
        switch (a.name) {
        case AttrName::MAC:
            macaddr = a.value;
            break;
        case AttrName::MTU:
            mtu = std::stoul(a.value);
            break;
        case AttrName::FLAGS:
            flags = std::stoul(a.value);
            break;
        default: //ignore others
            break;
        }
    }
    addrs = ni.getInetAddressList();
    return NetworkInterfaceInfo{ifname, ni.status(), macaddr, mtu, flags, addrs};
}

void ContainerImpl::createInterface(const std::string& hostif,
                                    const std::string& zoneif,
                                    InterfaceType type,
                                    MacVLanMode mode)
{
    NetworkInterface ni(zoneif, getInitPid());
    ni.create(type, hostif, mode);
}

void ContainerImpl::destroyInterface(const std::string& ifname)
{
    NetworkInterface ni(ifname, getInitPid());
    ni.destroy();
}

void ContainerImpl::moveInterface(const std::string& ifname)
{
    NetworkInterface ni(ifname);
    ni.moveToContainer(getInitPid());
}

void ContainerImpl::setUp(const std::string& ifname)
{
    NetworkInterface ni(ifname, getInitPid());
    ni.up();
}

void ContainerImpl::setDown(const std::string& ifname)
{
    NetworkInterface ni(ifname, getInitPid());
    ni.down();
}

void ContainerImpl::addInetAddr(const std::string& ifname, const InetAddr& addr)
{
    NetworkInterface ni(ifname, getInitPid());
    ni.addInetAddr(addr);
}

void ContainerImpl::delInetAddr(const std::string& ifname, const InetAddr& addr)
{
    NetworkInterface ni(ifname, getInitPid());
    ni.delInetAddr(addr);
}

void ContainerImpl::declareFile(const provision::File::Type type,
                                const std::string& path,
                                const int32_t flags,
                                const int32_t mode)
{
    provision::File newFile({type, path, flags, mode});
    mConfig.mProvisions.addFile(newFile);
    // TODO: update guard config

    if (isRunning()) {
        ProvisionFile fileCmd(mConfig, newFile);
        fileCmd.execute();
    }
}

const FileVector& ContainerImpl::getFiles() const
{
    return mConfig.mProvisions.getFiles();
}

void ContainerImpl::removeFile(const provision::File& item)
{
    mConfig.mProvisions.removeFile(item);

    if (isRunning()) {
        ProvisionFile fileCmd(mConfig, item);
        fileCmd.revert();
    }
}

void ContainerImpl::declareMount(const std::string& source,
                                 const std::string& target,
                                 const std::string& type,
                                 const int64_t flags,
                                 const std::string& data)
{
    provision::Mount newMount({source, target, type, flags, data});
    mConfig.mProvisions.addMount(newMount);
    // TODO: update guard config

    if (isRunning()) {
        ProvisionMount mountCmd(mConfig, newMount);
        mountCmd.execute();
    }
}

const MountVector& ContainerImpl::getMounts() const
{
    return mConfig.mProvisions.getMounts();
}

void ContainerImpl::removeMount(const provision::Mount& item)
{
    mConfig.mProvisions.removeMount(item);

    if (isRunning()) {
        ProvisionMount mountCmd(mConfig, item);
        mountCmd.revert();
    }
}

void ContainerImpl::declareLink(const std::string& source,
                                const std::string& target)
{
    provision::Link newLink({source, target});
    mConfig.mProvisions.addLink(newLink);
    // TODO: update guard config

    if (isRunning()) {
        ProvisionLink linkCmd(mConfig, newLink);
        linkCmd.execute();
    }
}

const LinkVector& ContainerImpl::getLinks() const
{
    return mConfig.mProvisions.getLinks();
}

void ContainerImpl::removeLink(const provision::Link& item)
{
    mConfig.mProvisions.removeLink(item);

    if (isRunning()) {
        ProvisionLink linkCmd(mConfig, item);
        linkCmd.revert();
    }
}

void ContainerImpl::addSubsystem(const std::string& name, const std::string& path)
{
    mConfig.mCgroups.subsystems.push_back(SubsystemConfig{name, path});
}

void ContainerImpl::addCGroup(const std::string& subsys,
                              const std::string& grpname,
                              const std::vector<CGroupParam>& comm,
                              const std::vector<CGroupParam>& params)
{
    mConfig.mCgroups.cgroups.push_back(CGroupConfig{subsys, grpname, comm, params});
}

} // namespace lxcpp
