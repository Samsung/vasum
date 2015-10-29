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

#include "lxcpp/utils.hpp"
#include "lxcpp/guard/guard.hpp"
#include "lxcpp/process.hpp"
#include "lxcpp/credentials.hpp"
#include "lxcpp/commands/prep-guest-terminal.hpp"
#include "lxcpp/commands/provision.hpp"
#include "lxcpp/commands/setup-userns.hpp"

#include "config/manager.hpp"
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

    Provisions provisions(config);
    provisions.execute();

    PrepGuestTerminal terminals(config.mTerminals);
    terminals.execute();

    if (config.mUserNSConfig.mUIDMaps.size()) {
        lxcpp::setreuid(0, 0);
    }
    if (config.mUserNSConfig.mGIDMaps.size()) {
        lxcpp::setregid(0, 0);
    }

    lxcpp::execve(config.mInit);

    return EXIT_FAILURE;
}

Guard::Guard(const std::string& socketPath)
    : mSignalFD(mEventPoll)
{
    mSignalFD.setHandler(SIGCHLD, std::bind(&Guard::onInitExit, this));

    // Setup socket communication
    mService.reset(new ipc::Service(mEventPoll, socketPath));

    using namespace std::placeholders;
    mService->setNewPeerCallback(std::bind(&Guard::onConnection, this, _1, _2));
    mService->setRemovedPeerCallback(std::bind(&Guard::onDisconnection, this, _1, _2));

    mService->setMethodHandler<api::Pid, api::Void>(api::METHOD_START,
            std::bind(&Guard::onStart, this, _1, _2, _3));
    mService->setMethodHandler<api::Pid, api::Void>(api::METHOD_STOP,
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

void Guard::onConnection(const ipc::PeerID& peerID, const ipc::FileDescriptor)
{
    if (!mPeerID.empty()) {
        // FIXME: Refuse connection
        LOGW("New Peer connected, but previous not disconnected");
    }
    mPeerID = peerID;
    mService->callAsyncFromCallback<api::Void, api::Void>(api::METHOD_GUARD_READY, mPeerID, std::shared_ptr<api::Void>());
}

void Guard::onDisconnection(const ipc::PeerID& peerID, const ipc::FileDescriptor)
{
    if(mPeerID != peerID) {
        LOGE("Unknown peerID: " << peerID);
    }
    mPeerID.clear();
}

void Guard::onInitExit()
{
    // TODO: Check PID if it's really init that died
    LOGD("Init died");

    if(mStopResult) {
        // TODO: Return the process's status if stop was initiated by calling stop()
        mStopResult->setVoid();
    }

    mService->stop();
}

void Guard::onSetConfig(const ipc::PeerID, std::shared_ptr<ContainerConfig>& data, ipc::MethodResult::Pointer result)
{
    mConfig = data;

    // TODO: We might use this command elsewhere, not only for a start
    logger::setupLogger(mConfig->mLogger.mType,
                        mConfig->mLogger.mLevel,
                        mConfig->mLogger.mArg);

    LOGD("Config & logging restored");

    try {
        LOGD("Setting the guard process title");
        const std::string title = "[LXCPP] " + mConfig->mName + " " + mConfig->mRootPath;
        setProcTitle(title);
    } catch (std::exception &e) {
        // Ignore, this is optional
        LOGW("Failed to set the guard process title: " << e.what());
    }

    result->setVoid();
}

void Guard::onGetConfig(const ipc::PeerID, std::shared_ptr<api::Void>&, ipc::MethodResult::Pointer result)
{
    LOGD("Sending out the config");
    result->set(mConfig);
}

void Guard::onStart(const ipc::PeerID, std::shared_ptr<api::Void>&, ipc::MethodResult::Pointer result)
{
    LOGI("Starting...");
    utils::Channel channel;
    ContainerData data(*mConfig, channel);

    const pid_t initPid = lxcpp::clone(startContainer,
                                       &data,
                                       mConfig->mNamespaces);
    mConfig->mInitPid = initPid;

    // TODO: container preparation part 2: things to do immediately after clone

    SetupUserNS userNS(mConfig->mUserNSConfig, mConfig->mInitPid);
    userNS.execute();

    // send continue sync to container once userns, netns, cgroups, etc, are configured
    channel.setLeft();
    channel.write(true);
    channel.shutdown();

    // Configuration succeed, return the init's PID
    auto ret = std::make_shared<api::Pid>(initPid);
    result->set(ret);
}

void Guard::onStop(const ipc::PeerID, std::shared_ptr<api::Void>&, ipc::MethodResult::Pointer result)
{
    LOGI("Stopping...");

    // TODO: Use initctl/systemd-initctl if available in container
    utils::sendSignal(mConfig->mInitPid, SIGTERM);

    mStopResult = result;
}

int Guard::execute()
{
    // TODO: container preparation part 1: things to do before clone

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
