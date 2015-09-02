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

pid_t ContainerImpl::getInitPid() const
{
    return mInitPid;
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

void ContainerImpl::attach(Container::AttachCall& call,
                           const std::string& cwdInContainer)
{
    Attach attach(*this,
                  call,
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
