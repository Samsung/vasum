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
 * @author  Lukasz Pawelczyk (l.pawelczyk@samsung.com)
 * @brief   LXCPP guard process implementation
 */

#include "config.hpp"

#include "lxcpp/utils.hpp"
#include "lxcpp/guard/guard.hpp"
#include "lxcpp/process.hpp"
#include "lxcpp/credentials.hpp"
#include "lxcpp/hostname.hpp"
#include "lxcpp/rlimit.hpp"
#include "lxcpp/sysctl.hpp"
#include "lxcpp/commands/prep-guest-terminal.hpp"
#include "lxcpp/commands/provision.hpp"
#include "lxcpp/commands/setup-userns.hpp"
#include "lxcpp/commands/cgroups.hpp"
#include "lxcpp/commands/netcreate.hpp"

#include "logger/logger.hpp"
#include "utils/signal.hpp"

#include <unistd.h>
#include <sys/wait.h>

namespace lxcpp {

int Guard::startContainer(void* data)
{
    ContainerConfig& config = static_cast<ContainerData*>(data)->mConfig;
    utils::Channel& channel = static_cast<ContainerData*>(data)->mChannel;

    // wait for continue sync from guard
    channel.setRight();
    channel.read<bool>();
    channel.shutdown();

    // TODO: container preparation part 3: things to do in the container process

    lxcpp::setHostName(config.mHostName);

    Provisions provisions(config);
    provisions.execute();

    PrepGuestTerminal terminals(config.mTerminals);
    terminals.execute();

    if (!config.mUserNSConfig.mUIDMaps.empty()) {
        lxcpp::setreuid(0, 0);
    }
    if (!config.mUserNSConfig.mGIDMaps.empty()) {
        lxcpp::setregid(0, 0);
    }

    NetConfigureAll network(config.mNetwork);
    network.execute();

    if (!config.mRlimits.empty()) {
        for (const auto& limit : config.mRlimits) {
            lxcpp::setRlimit(std::get<0>(limit), std::get<1>(limit),std::get<2>(limit));
        }
    }

    if (!config.mKernelParameters.empty()) {
        for (const auto& sysctl : config.mKernelParameters) {
            lxcpp::writeKernelParameter(sysctl.first, sysctl.second);
        }
    }

    utils::CArgsBuilder args;
    lxcpp::execve(args.add(config.mInit));

    return EXIT_FAILURE;
}

Guard::Guard(const std::string& socketPath)
    : mSignalFD(mEventPoll)
{
    using namespace std::placeholders;
    mSignalFD.setHandler(SIGCHLD, std::bind(&Guard::onInitExit, this, _1));

    // Setup socket communication
    mService.reset(new cargo::ipc::Service(mEventPoll, socketPath));

    mService->setRemovedPeerCallback(std::bind(&Guard::onDisconnection, this, _1, _2));
    mService->setNewPeerCallback(std::bind(&Guard::onConnection, this, _1, _2));

    mService->setMethodHandler<api::Pid, api::Void>(api::METHOD_START,
            std::bind(&Guard::onStart, this, _1, _2, _3));
    mService->setMethodHandler<api::Void, api::Void>(api::METHOD_STOP,
            std::bind(&Guard::onStop, this, _1, _2, _3));
    mService->setMethodHandler<api::Void, ContainerConfig>(api::METHOD_SET_CONFIG,
            std::bind(&Guard::onSetConfig, this, _1, _2, _3));
    mService->setMethodHandler<ContainerConfig, api::Void>(api::METHOD_GET_CONFIG,
            std::bind(&Guard::onGetConfig, this, _1, _2, _3));

    mService->start();
}

Guard::~Guard()
{
}

void Guard::onConnection(const cargo::ipc::PeerID& peerID, const cargo::ipc::FileDescriptor)
{
    LOGT("onConnection");

    if (!mPeerID.empty()) {
        // FIXME: Refuse connection
        LOGW("New Peer connected, but previous not disconnected");
    }
    mPeerID = peerID;

    if (!mConfig) {
        // Host is connecting to a STOPPED container,
        // it needs to s setup and start it
        mService->callAsyncFromCallback<api::Void, api::Void>(api::METHOD_GUARD_READY, mPeerID, std::shared_ptr<api::Void>());
    } else {
        // Host is connecting to a RUNNING container
        // Guard passes info about the started Init
        mService->callAsyncFromCallback<ContainerConfig, api::Void>(api::METHOD_GUARD_CONNECTED, mPeerID, mConfig);
    }
}

void Guard::onDisconnection(const cargo::ipc::PeerID& peerID, const cargo::ipc::FileDescriptor)
{
    LOGT("onDisconnection");

    if(mPeerID != peerID) {
        LOGE("Unknown peerID: " << peerID);
    }
    mPeerID.clear();
}

void Guard::onInitExit(struct ::signalfd_siginfo& sigInfo)
{
    LOGT("onInitExit");

    if(mConfig->mInitPid != static_cast<int>(sigInfo.ssi_pid)) {
        return;
    }

    LOGD("Init died");
    mConfig->mState = Container::State::STOPPED;

    auto data = std::make_shared<api::ExitStatus>(sigInfo.ssi_status);
    mService->callAsync<api::ExitStatus, api::Void>(api::METHOD_INIT_STOPPED, mPeerID, data);

    mService->stop(false);
}

cargo::ipc::HandlerExitCode Guard::onSetConfig(const cargo::ipc::PeerID,
                                               std::shared_ptr<ContainerConfig>& data,
                                               cargo::ipc::MethodResult::Pointer result)
{
    LOGT("onSetConfig");

    mConfig = data;

    try {
        logger::setupLogger(mConfig->mLogger.mType,
                            mConfig->mLogger.mLevel,
                            mConfig->mLogger.mArg);
        LOGD("Config & logging restored");
    }
    catch(const std::exception& e) {
        result->setError(api::GUARD_SET_CONFIG_ERROR, e.what());
        return cargo::ipc::HandlerExitCode::SUCCESS;
    }

    result->setVoid();
    return cargo::ipc::HandlerExitCode::SUCCESS;
}

cargo::ipc::HandlerExitCode Guard::onGetConfig(const cargo::ipc::PeerID,
                                               std::shared_ptr<api::Void>&,
                                               cargo::ipc::MethodResult::Pointer result)
{
    LOGT("onGetConfig");

    result->set(mConfig);
    return cargo::ipc::HandlerExitCode::SUCCESS;
}

cargo::ipc::HandlerExitCode Guard::onStart(const cargo::ipc::PeerID,
                                           std::shared_ptr<api::Void>&,
                                           cargo::ipc::MethodResult::Pointer result)
{
    LOGT("onStart");

    mConfig->mState = Container::State::STARTING;

    try {
        LOGD("Setting the guard process title");
        const std::string title = "[LXCPP] " + mConfig->mName + " " + mConfig->mRootPath;
        setProcTitle(title);
    } catch (std::exception &e) {
        // Ignore, this is optional
        LOGW("Failed to set the guard configuration: " << e.what());
    }

    // TODO: container preparation part 1: things to do before clone
    CGroupMakeAll cgroups(mConfig->mCgroups);
    cgroups.execute();

    utils::Channel channel;
    ContainerData data(*mConfig, channel);

    mConfig->mInitPid = lxcpp::clone(startContainer,
                                     &data,
                                     mConfig->mNamespaces);

    // TODO: container preparation part 2: things to do immediately after clone

    NetCreateAll network(mConfig->mNetwork, mConfig->mInitPid);
    network.execute();

    SetupUserNS userNS(mConfig->mUserNSConfig, mConfig->mInitPid);
    userNS.execute();

    CGroupAssignPidAll cgroupAssignPid(mConfig->mCgroups, mConfig->mInitPid);
    cgroupAssignPid.execute();

    // send continue sync to container once userns, netns, cgroups, etc, are configured
    channel.setLeft();
    channel.write(true);
    channel.shutdown();

    // Init started, change state
    mConfig->mState = Container::State::RUNNING;

    // Configuration succeed, return the init's PID
    auto ret = std::make_shared<api::Pid>(mConfig->mInitPid);
    result->set(ret);
    return cargo::ipc::HandlerExitCode::SUCCESS;
}

cargo::ipc::HandlerExitCode Guard::onStop(const cargo::ipc::PeerID,
                                          std::shared_ptr<api::Void>&,
                                          cargo::ipc::MethodResult::Pointer result)
{
    LOGT("onStop");
    LOGI("Stopping...");

    mConfig->mState = Container::State::STOPPING;

    // TODO: Use initctl/systemd-initctl if available in container
    utils::sendSignal(mConfig->mInitPid, SIGTERM);

    result->setVoid();
    return cargo::ipc::HandlerExitCode::SUCCESS;
}

int Guard::execute()
{
    // Polling loop
    while (mService->isStarted()) {
        mEventPoll.dispatchIteration(-1);
    }

    if (!mConfig) {
        // Config wasn't set, nothing started, fail
        return EXIT_FAILURE;
    }

    int status = lxcpp::waitpid(mConfig->mInitPid);
    LOGD("Init exited with status: " << status);

    // TODO: container (de)preparation part 4: cleanup after container quits

    Provisions provisions(*mConfig);
    provisions.revert();

    return status;
}

} // namespace lxcpp
