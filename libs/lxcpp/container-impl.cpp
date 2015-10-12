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
#include "lxcpp/commands/start.hpp"
#include "lxcpp/commands/stop.hpp"
#include "lxcpp/commands/prep-host-terminal.hpp"

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


namespace {

// TODO: UGLY: REMOVEME:
// It will be removed as soon as Container::console() will get implemented
// I need it for now to know I didn't brake anything. It will be eradicated.
void readTerminal(const lxcpp::TerminalConfig &term)
{
    char *buf = NULL;
    size_t size = 0;
    ssize_t ret;

    printf("%s output:\n", term.ptsName.c_str());
    usleep(10000);

    FILE *fp = fdopen(term.masterFD.value, "r");
    while((ret = getline(&buf, &size, fp)) != -1L) {
        printf("%s", buf);
    }
    free(buf);
}

} // namespace

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

void ContainerImpl::setTerminalCount(const unsigned int count)
{
    if (count == 0) {
        const std::string msg = "Container needs at least one terminal";
        LOGE(msg);
        throw ConfigureException(msg);
    }

    mConfig.mTerminals.count = count;
}

void ContainerImpl::setNamespaces(const int namespaces)
{
    mConfig.mNamespaces = namespaces;
}

void ContainerImpl::start()
{
    // TODO: check config consistency and completeness somehow

    PrepHostTerminal terminal(mConfig.mTerminals);
    terminal.execute();

    Start start(mConfig);
    start.execute();

    // TODO: UGLY: REMOVEME: read from 1st terminal
    readTerminal(mConfig.mTerminals.PTYs[0]);
}

void ContainerImpl::stop()
{
    // TODO: things to do when shuttting down the container:
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
    Attach attach(*this,
                  argv,
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
                                       const std::vector<InetAddr>& addrs,
                                       MacVLanMode mode)
{
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
    Attrs attrs = ni.getAttrs();
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

} // namespace lxcpp
