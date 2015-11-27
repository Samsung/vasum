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
#include "utils/fs.hpp"
#include "utils/paths.hpp"
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

ContainerImpl::ContainerImpl(const std::string &name,
                             const std::string &rootPath,
                             const std::string &workPath)
    : mConfig(new ContainerConfig()),
      mInotify(mDispatcher.getPoll())
{
    // Validate arguments
    if (name.empty()) {
        const std::string msg = "Name cannot be empty";
        LOGE(msg);
        throw ConfigureException(msg);
    }

    utils::assertIsDir(rootPath);
    utils::assertIsDir(workPath);

    // Fill known configuration
    mConfig->mName = name;
    mConfig->mRootPath = rootPath;
    mConfig->mNamespaces = CLONE_NEWIPC | CLONE_NEWNS | CLONE_NEWPID | CLONE_NEWUTS;
    mConfig->mSocketPath = utils::createFilePath(workPath, name, ".socket");

    using namespace std::placeholders;

    // IPC with the Guard process
    mClient.reset(new cargo::ipc::Client(mDispatcher.getPoll(), mConfig->mSocketPath));
    mClient->setMethodHandler<api::Void, api::ExitStatus>(api::METHOD_INIT_STOPPED,
            std::bind(&ContainerImpl::onInitStopped, this, _1, _2, _3));
    mClient->setMethodHandler<api::Void, api::Void>(api::METHOD_GUARD_READY,
            std::bind(&ContainerImpl::onGuardReady, this, _1, _2, _3));

    // TODO: Connect to a running Guard with something like this:
    // try {
    //     // If there's a container running try to connect.
    //     // TODO: this might need some named semaphore to prevent races between two processes using this library.
    //     mClient->start();
    //     // TODO: Get the configuration from the running Guard
    // } catch(const cargo::ipc::IPCException&) {
    //     // It's OK, container isn't yet started.
    // }

    // Watch the workdir for filesystem events
    mInotify.setHandler(workPath, IN_CREATE | IN_DELETE | IN_ISDIR, std::bind(&ContainerImpl::onWorkFileEvent, this, _1, _2));
}

ContainerImpl::~ContainerImpl()
{
    try {
        stop();
    } catch(std::exception& e) {
        LOGW("Discarding an error during stopping: " << e.what());
    }
    mClient->stop(true);
}

void ContainerImpl::onWorkFileEvent(const std::string& name, const uint32_t mask)
{
    Lock lock(mStateMutex);

    if(mask & IN_CREATE) {
        if (name == mConfig->mName + ".socket") {
            mClient->start();
        }
    } else if (mask & IN_DELETE) {
        if (name == mConfig->mName + ".socket") {
            LOGW("Container's socket deleted");
        }
    }
}

const std::string& ContainerImpl::getName() const
{
    Lock lock(mStateMutex);

    return mConfig->mName;
}

const std::string& ContainerImpl::getRootPath() const
{

    Lock lock(mStateMutex);

    return mConfig->mRootPath;
}

void ContainerImpl::setHostName(const std::string& /*hostname*/)
{
    throw NotImplementedException();
}

const std::vector<std::string>& ContainerImpl::getInit()
{
    Lock lock(mStateMutex);

    return mConfig->mInit;
}

void ContainerImpl::setInit(const std::vector<std::string> &init)
{
    Lock lock(mStateMutex);

    if (init.empty() || init[0].empty()) {
        const std::string msg = "Init path cannot be empty";
        LOGE(msg);
        throw ConfigureException(msg);
    }

    std::string path = mConfig->mRootPath + "/" + init[0];

    if (::access(path.c_str(), X_OK) < 0) {
        const std::string msg = "Init path must point to an executable file";
        LOGE(msg);
        throw ConfigureException(msg);
    }

    mConfig->mInit = init;
}

pid_t ContainerImpl::getGuardPid() const
{
    Lock lock(mStateMutex);

    return mConfig->mGuardPid;
}

pid_t ContainerImpl::getInitPid() const
{
    Lock lock(mStateMutex);

    return mConfig->mInitPid;
}

void ContainerImpl::setLogger(const logger::LogType type,
                              const logger::LogLevel level,
                              const std::string &arg)
{
    Lock lock(mStateMutex);

    mConfig->mLogger.set(type, level, arg);
}

void ContainerImpl::setTerminalCount(const unsigned int count)
{
    Lock lock(mStateMutex);

    if (count == 0) {
        const std::string msg = "Container needs at least one terminal";
        LOGE(msg);
        throw ConfigureException(msg);
    }

    mConfig->mTerminals.count = count;
}

void ContainerImpl::addUIDMap(unsigned min, unsigned max, unsigned num)
{
    Lock lock(mStateMutex);

    mConfig->mNamespaces |= CLONE_NEWUSER;

    if (mConfig->mUserNSConfig.mUIDMaps.size() >= 5) {
        const std::string msg = "Max number of 5 UID mappings has been already reached";
        LOGE(msg);
        throw ConfigureException(msg);
    }

    mConfig->mUserNSConfig.mUIDMaps.emplace_back(min, max, num);
}

void ContainerImpl::addGIDMap(unsigned min, unsigned max, unsigned num)
{
    Lock lock(mStateMutex);

    mConfig->mNamespaces |= CLONE_NEWUSER;

    if (mConfig->mUserNSConfig.mGIDMaps.size() >= 5) {
        const std::string msg = "Max number of 5 GID mappings has been already reached";
        LOGE(msg);
        throw ConfigureException(msg);
    }

    mConfig->mUserNSConfig.mGIDMaps.emplace_back(min, max, num);
}

void ContainerImpl::start()
{
    Lock lock(mStateMutex);

    // TODO: check config consistency and completeness somehow
    if(mConfig->mState != Container::State::STOPPED) {
        throw ForbiddenActionException("Container isn't stopped, can't start");
    }
    mConfig->mState = Container::State::STARTING;

    PrepHostTerminal terminal(mConfig->mTerminals);
    terminal.execute();

    Start start(mConfig);
    start.execute();
}

bool ContainerImpl::onGuardReady(const cargo::ipc::PeerID,
                                 std::shared_ptr<api::Void>&,
                                 cargo::ipc::MethodResult::Pointer methodResult)
{
    Lock lock(mStateMutex);

    // Guard is up

    mClient->callAsyncFromCallback<ContainerConfig, api::Void>(api::METHOD_SET_CONFIG, mConfig);

    auto initStarted = [this](cargo::ipc::Result<api::Pid>&& result) {
        Lock lock(mStateMutex);

        if (!result.isValid()) {
            LOGE("Failed to get init's PID");
            result.rethrow();
        }

        auto initPid = result.get();
        mConfig->mInitPid = initPid->value;
        LOGI("Init PID: " << initPid->value);
        if (mConfig->mInitPid <= 0) {
            // TODO: Handle the error
            LOGE("Bad Init PID");
            return;
        }

        mConfig->mState = Container::State::RUNNING;
        if (mStartedCallback) {
            mStartedCallback();
        }
    };

    mClient->callAsyncFromCallback<api::Void, api::Pid>(api::METHOD_START, std::shared_ptr<api::Void>(), initStarted);

    methodResult->setVoid();

    return true;
}

void ContainerImpl::stop()
{
    {
        Lock lock(mStateMutex);

        // TODO: things to do when shutting down the container:
        // - close PTY master FDs from the config so we won't keep PTYs open
        if(mConfig->mState != Container::State::RUNNING) {
            throw ForbiddenActionException("Container isn't running, can't stop");
        }
        mConfig->mState = Container::State::STOPPING;
    }

    Stop stop(mConfig, mClient);
    stop.execute();
}

bool ContainerImpl::onInitStopped(const cargo::ipc::PeerID,
                                  std::shared_ptr<api::ExitStatus>& data,
                                  cargo::ipc::MethodResult::Pointer methodResult)
{
    Lock lock(mStateMutex);

    // Guard detected that Init exited
    mConfig->mExitStatus = data->value;
    LOGI("STOPPED " << mConfig->mName << " Exit status: " << mConfig->mExitStatus);

    mConfig->mState = Container::State::STOPPED;
    if (mStoppedCallback) {
        mStoppedCallback();
    }

    methodResult->setVoid();
    return true;
}

void ContainerImpl::freeze()
{
    Lock lock(mStateMutex);

    // TODO: Add a FREEZED, FREEZING state
    throw NotImplementedException();
}

void ContainerImpl::unfreeze()
{
    Lock lock(mStateMutex);

    throw NotImplementedException();
}

void ContainerImpl::reboot()
{
    Lock lock(mStateMutex);

    // TODO: Handle container states
    throw NotImplementedException();
}

Container::State ContainerImpl::getState()
{
    Lock lock(mStateMutex);

    return mConfig->mState;
}

void ContainerImpl::setStartedCallback(const Container::Callback& callback)
{
    Lock lock(mStateMutex);

    mStartedCallback = callback;
}

void ContainerImpl::setStoppedCallback(const Container::Callback& callback)
{
    Lock lock(mStateMutex);

    mStoppedCallback = callback;
}

int ContainerImpl::attach(const std::vector<std::string>& argv,
                          const uid_t uid,
                          const gid_t gid,
                          const std::string& ttyPath,
                          const std::vector<gid_t>& supplementaryGids,
                          const int capsToKeep,
                          const std::string& workDirInContainer,
                          const std::vector<std::string>& envToKeep,
                          const std::vector<std::pair<std::string, std::string>>& envToSet)
{
    Lock lock(mStateMutex);

    if(mConfig->mState != Container::State::RUNNING) {
        throw ForbiddenActionException("Container isn't running, can't attach");
    }

    std::vector<std::pair<std::string, std::string>> envToSetFinal = {{"container","lxcpp"}};
    envToSetFinal.insert(envToSetFinal.end(), envToSet.begin(), envToSet.end());

    Attach attach(*mConfig,
                  argv,
                  uid,
                  gid,
                  ttyPath,
                  supplementaryGids,
                  capsToKeep,
                  workDirInContainer,
                  envToKeep,
                  envToSetFinal,
                  mConfig->mLogger);
    // TODO: Env variables should agree with the ones already in the container
    attach.execute();
    return attach.getExitCode();
}

void ContainerImpl::console()
{
    Lock lock(mStateMutex);

    Console console(mConfig->mTerminals);
    console.execute();
}

void ContainerImpl::addInterfaceConfig(InterfaceConfigType type,
                                       const std::string& hostif,
                                       const std::string& zoneif,
                                       const std::vector<InetAddr>& addrs,
                                       MacVLanMode mode)
{
    Lock lock(mStateMutex);

    mConfig->mNamespaces |= CLONE_NEWNET;
    mConfig->mNetwork.addInterfaceConfig(type, hostif, zoneif, addrs, mode);
}

void ContainerImpl::addInetConfig(const std::string& ifname, const InetAddr& addr)
{
    Lock lock(mStateMutex);

    mConfig->mNetwork.addInetConfig(ifname, addr);
}

std::vector<std::string> ContainerImpl::getInterfaces() const
{
    Lock lock(mStateMutex);

    return NetworkInterface::getInterfaces(mConfig->mInitPid);
}

NetworkInterfaceInfo ContainerImpl::getInterfaceInfo(const std::string& ifname) const
{
    Lock lock(mStateMutex);

    NetworkInterface ni(ifname, mConfig->mInitPid);
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
    Lock lock(mStateMutex);

    NetworkInterface ni(zoneif, mConfig->mInitPid);
    ni.create(type, hostif, mode);
}

void ContainerImpl::destroyInterface(const std::string& ifname)
{
    Lock lock(mStateMutex);

    NetworkInterface ni(ifname, mConfig->mInitPid);
    ni.destroy();
}

void ContainerImpl::moveInterface(const std::string& ifname)
{
    Lock lock(mStateMutex);

    NetworkInterface ni(ifname);
    ni.moveToContainer(mConfig->mInitPid);
}

void ContainerImpl::setUpInterface(const std::string& ifname)
{
    Lock lock(mStateMutex);

    NetworkInterface ni(ifname, mConfig->mInitPid);
    ni.up();
}

void ContainerImpl::setDownInterface(const std::string& ifname)
{
    Lock lock(mStateMutex);

    NetworkInterface ni(ifname, mConfig->mInitPid);
    ni.down();
}

void ContainerImpl::addInetAddr(const std::string& ifname, const InetAddr& addr)
{
    Lock lock(mStateMutex);

    NetworkInterface ni(ifname, mConfig->mInitPid);
    ni.addInetAddr(addr);
}

void ContainerImpl::delInetAddr(const std::string& ifname, const InetAddr& addr)
{
    Lock lock(mStateMutex);

    NetworkInterface ni(ifname, mConfig->mInitPid);
    ni.delInetAddr(addr);
}

void ContainerImpl::declareFile(const provision::File::Type type,
                                const std::string& path,
                                const int32_t flags,
                                const int32_t mode)
{
    Lock lock(mStateMutex);

    provision::File newFile({type, path, flags, mode});
    mConfig->mProvisions.addFile(newFile);
    // TODO: update guard config

    if (mConfig->mState == Container::State::RUNNING) {
        ProvisionFile fileCmd(newFile);
        fileCmd.execute();
    }
}

const FileVector& ContainerImpl::getFiles() const
{
    Lock lock(mStateMutex);

    return mConfig->mProvisions.getFiles();
}

void ContainerImpl::removeFile(const provision::File& item)
{
    Lock lock(mStateMutex);

    mConfig->mProvisions.removeFile(item);

    if (mConfig->mState == Container::State::RUNNING) {
        ProvisionFile fileCmd(item);
        fileCmd.revert();
    }
}

void ContainerImpl::declareMount(const std::string& source,
                                 const std::string& target,
                                 const std::string& type,
                                 const int64_t flags,
                                 const std::string& data)
{
    Lock lock(mStateMutex);

    provision::Mount newMount({source, target, type, flags, data});
    mConfig->mProvisions.addMount(newMount);
    // TODO: update guard config

    if (mConfig->mState == Container::State::RUNNING) {
        ProvisionMount mountCmd(newMount);
        mountCmd.execute();
    }
}

const MountVector& ContainerImpl::getMounts() const
{
    Lock lock(mStateMutex);

    return mConfig->mProvisions.getMounts();
}

void ContainerImpl::removeMount(const provision::Mount& item)
{
    Lock lock(mStateMutex);

    mConfig->mProvisions.removeMount(item);

    if (mConfig->mState == Container::State::RUNNING) {
        ProvisionMount mountCmd(item);
        mountCmd.revert();
    }
}

void ContainerImpl::declareLink(const std::string& source,
                                const std::string& target)
{
    Lock lock(mStateMutex);

    provision::Link newLink({source, target});
    mConfig->mProvisions.addLink(newLink);
    // TODO: update guard config

    if (mConfig->mState == Container::State::RUNNING) {
        ProvisionLink linkCmd(newLink);
        linkCmd.execute();
    }
}

const LinkVector& ContainerImpl::getLinks() const
{
    Lock lock(mStateMutex);

    return mConfig->mProvisions.getLinks();
}

void ContainerImpl::removeLink(const provision::Link& item)
{
    Lock lock(mStateMutex);

    mConfig->mProvisions.removeLink(item);

    if (mConfig->mState == Container::State::RUNNING) {
        ProvisionLink linkCmd(item);
        linkCmd.revert();
    }
}

void ContainerImpl::addSubsystem(const std::string& name, const std::string& path)
{
    Lock lock(mStateMutex);

    mConfig->mCgroups.subsystems.push_back(SubsystemConfig{name, path});
}

void ContainerImpl::addCGroup(const std::string& subsys,
                              const std::string& grpname,
                              const std::vector<CGroupParam>& comm,
                              const std::vector<CGroupParam>& params)
{
    Lock lock(mStateMutex);

    mConfig->mCgroups.cgroups.push_back(CGroupConfig{subsys, grpname, comm, params});
}

void ContainerImpl::setEnv(const std::vector<std::pair<std::string, std::string>>& /*variables*/)
{
    throw NotImplementedException();
}

void ContainerImpl::setCaps(const int /*caps*/)
{
    throw NotImplementedException();
}

void ContainerImpl::setSystemProperty(const std::string& /*name*/, const std::string& /*value*/)
{
    throw NotImplementedException();
}

void ContainerImpl::setRlimit(const std::string& /*type*/, const uint64_t /*hard*/, const uint64_t /*soft*/)
{
    throw NotImplementedException();
}

void ContainerImpl::setNamespaces(const int /*namespaces*/)
{
    throw NotImplementedException();
}

void ContainerImpl::setUser(const int /*uid*/, const int /*gid*/, const std::vector<int> /*additionalGids*/)
{
    throw NotImplementedException();
}

void ContainerImpl::addDevice(const std::string& /*path*/,
                              const char /*type*/,
                              const int64_t /*major*/,
                              const int64_t /*minor*/,
                              const std::string& /*permissions*/,
                              const uint32_t /*fileMode*/,
                              const uint32_t /*uid*/,
                              const uint32_t /*gid*/)
{
    throw NotImplementedException();
}

void ContainerImpl::addHook(const std::string& /*type*/,
                            const std::vector<std::string>& /*hook*/,
                            const std::vector<std::pair<std::string, std::string>>& /*env*/)
{
    throw NotImplementedException();
}

} // namespace lxcpp
