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
#include "lxcpp/commands/attach.hpp"
#include "lxcpp/commands/start.hpp"

#include "logger/logger.hpp"
#include "utils/exception.hpp"

#include <unistd.h>
#include <sys/mount.h>
#include <sys/wait.h>
#include <mutex>
#include <algorithm>


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

void ContainerImpl::start()
{
    // TODO: check config consistency and completeness somehow
    Start start(mConfig);
    start.execute();
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

void ContainerImpl::attach(Container::AttachCall& call,
                           const std::string& cwdInContainer)
{
    Attach attach(*this,
                  call,
                  /*uid in container*/ 0,
                  /*gid in container*/ 0,
                  "/dev/tty",
                  /*supplementary gids in container*/ {},
                  /*capsToKeep*/ 0,
                  cwdInContainer,
                  /*envToKeep*/ {},
                  /*envInContainer*/ {{"container","lxcpp"}});
    // TODO: Env variables should agree with the ones already in the container
    attach.execute();
}

const std::vector<Namespace>& ContainerImpl::getNamespaces() const
{
    return mNamespaces;
}

void ContainerImpl::addInterfaceConfig(const std::string& hostif,
                                       const std::string& zoneif,
                                       InterfaceType type,
                                       MacVLanMode mode)
{
    mInterfaceConfig.push_back(NetworkInterfaceConfig(hostif,zoneif,type,mode));
}

void ContainerImpl::addAddrConfig(const std::string& /*ifname*/, const InetAddr& /*addr*/)
{
    throw NotImplementedException();
}

std::vector<std::string> ContainerImpl::getInterfaces()
{
    return NetworkInterface::getInterfaces(getInitPid());
}

NetworkInterfaceInfo ContainerImpl::getInterfaceInfo(const std::string& /*ifname*/)
{
    throw NotImplementedException();
}

void ContainerImpl::createInterface(const std::string& hostif,
                                    const std::string& zoneif,
                                    InterfaceType type,
                                    MacVLanMode mode)
{
    NetworkInterface ni(*this, zoneif);
    ni.create(hostif, type, mode);
}

void ContainerImpl::destroyInterface(const std::string& /*ifname*/)
{
    throw NotImplementedException();
}

void ContainerImpl::setUp(const std::string& /*ifname*/)
{
    throw NotImplementedException();
}

void ContainerImpl::setDown(const std::string& /*ifname*/)
{
    throw NotImplementedException();
}

void ContainerImpl::addAddr(const std::string& /*ifname*/, const InetAddr& /*addr*/)
{
    throw NotImplementedException();
}

void ContainerImpl::delAddr(const std::string& /*ifname*/, const InetAddr& /*addr*/)
{
    throw NotImplementedException();
}

} // namespace lxcpp
