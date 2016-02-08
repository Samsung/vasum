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
#include "lxcpp/hostname.hpp"
#include "lxcpp/rlimit.hpp"
#include "lxcpp/sysctl.hpp"
#include "lxcpp/capability.hpp"
#include "lxcpp/environment.hpp"
#include "lxcpp/filesystem.hpp"
#include "lxcpp/terminal.hpp"
#include "lxcpp/commands/prep-pty-terminal.hpp"
#include "lxcpp/commands/prep-guest-terminal.hpp"
#include "lxcpp/commands/provision.hpp"
#include "lxcpp/commands/setup-userns.hpp"
#include "lxcpp/commands/setup-smackns.hpp"
#include "lxcpp/commands/cgroups.hpp"
#include "lxcpp/commands/netcreate.hpp"
#include "lxcpp/commands/prep-dev-fs.hpp"
#include "lxcpp/commands/pivot-and-prep-root.hpp"

#include "logger/logger.hpp"
#include "utils/fs.hpp"
#include "utils/signal.hpp"
#include "utils/paths.hpp"
#include "utils/credentials.hpp"

#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/wait.h>

namespace lxcpp {

void Guard::containerPrepPreClone()
{
    PrepDevFS devFS(*mConfig);
    devFS.execute();

    PrepPTYTerminal ptys(mGuardPTYs);
    ptys.execute();

    Provisions provisions(*mConfig);
    provisions.execute();

    CGroupMakeAll cgroups(mConfig->mCgroups);
    cgroups.execute();

    mContToImpl.resize(mGuardPTYs.mCount);
    mImplToCont.resize(mGuardPTYs.mCount);
    mContToImplOffset.resize(mGuardPTYs.mCount);
    mImplToContOffset.resize(mGuardPTYs.mCount);

    using namespace std::placeholders;
    for (unsigned i = 0; i < mGuardPTYs.mPTYs.size(); ++i) {
        int contFD = mGuardPTYs.mPTYs[i].mMasterFD.value;
        int implFD = utils::open(mConfig->mTerminals.mPTYs[i].mPtsName, O_RDWR|O_NOCTTY|O_NONBLOCK|O_CLOEXEC);
        mImplSlaveFDs.push_back(implFD);

        mContToImplOffset[i] = 0;
        mImplToContOffset[i] = 0;

        mEventPoll.addFD(contFD, EPOLLIN, std::bind(&Guard::onContTerminal, this, i, _1, _2));
        mEventPoll.addFD(implFD, EPOLLIN, std::bind(&Guard::onImplTerminal, this, i, _1, _2));
    }
}

void Guard::containerPrepPostClone()
{
    SetupUserNS userNS(mConfig->mUserNSConfig, mConfig->mInitPid);
    userNS.execute();

    NetCreateAll network(mConfig->mNetwork, mConfig->mInitPid);
    network.execute();

    SetupSmackNS smackNS(mConfig->mSmackNSConfig, mConfig->mInitPid);
    smackNS.execute();

    CGroupAssignPidAll cgroupAssignPid(mConfig->mCgroups, mConfig->mInitPid);
    cgroupAssignPid.execute();
}

void Guard::containerPrepInClone(ContainerConfig &config)
{
    lxcpp::setHostName(config.mHostName);

    // After this command the previous root FS is still mounted in /.oldroot
    PivotAndPrepRoot root(config);
    root.execute();

    PrepGuestTerminal terminals(config.mTerminals);
    terminals.execute();

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

    lxcpp::dropCapsFromBoundingExcept(config.mCapsToKeep);

    lxcpp::clearenv();
    lxcpp::setenv(config.mEnvToSet);

    // Remove /.oldroot only after all the commands have finished, they might've needed it
    lxcpp::umountSubtree(config.mOldRoot);
    utils::rmdir(config.mOldRoot);
}

void Guard::containerCleanup()
{
    for (unsigned i = 0; i < mGuardPTYs.mPTYs.size(); ++i) {
        int contFD = mGuardPTYs.mPTYs[i].mMasterFD.value;
        int implFD = mImplSlaveFDs[i];

        mEventPoll.removeFD(contFD);
        mEventPoll.removeFD(implFD);
    }

    Provisions provisions(*mConfig);
    provisions.revert();

    PrepPTYTerminal ptys(mGuardPTYs);
    ptys.revert();

    PrepDevFS devFS(*mConfig);
    devFS.revert();
}

int Guard::startContainer(void* data)
{
    ContainerConfig& config = static_cast<ContainerData*>(data)->mConfig;
    utils::Channel& channel = static_cast<ContainerData*>(data)->mChannel;

    // Brackets are here to call destructors before execv.
    {
        // Wait for continue sync from guard
        channel.setRight();
        channel.read<bool>();

        utils::setregid(0, 0);
        utils::setgroups(std::vector<gid_t>());
        utils::setreuid(0, 0);

        containerPrepInClone(config);

        // Notify that Init's preparation is done
        channel.write(true);
        channel.shutdown();
    }

    utils::CArgsBuilder args;
    lxcpp::execv(args.add(config.mInit));

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
    mService->setMethodHandler<api::Void, api::Int>(api::METHOD_RESIZE_TERM,
            std::bind(&Guard::onResizeTerm, this, _1, _2, _3));

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
        LOGE("Unknown peerID: " << cargo::ipc::shortenPeerID(peerID));
    }
    mPeerID.clear();
}

void Guard::onInitExit(struct ::signalfd_siginfo& sigInfo)
{
    LOGT("onInitExit");

    if(mConfig->mInitPid != static_cast<int>(sigInfo.ssi_pid)) {
        return;
    }

    LOGD("Init died, cleaning up");

    containerCleanup();

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
    mGuardPTYs.mCount = mConfig->mTerminals.mCount;
    mGuardPTYs.mUID = mConfig->mUserNSConfig.getContainerRootUID();
    mGuardPTYs.mDevptsPath = utils::createFilePath(mConfig->mWorkPath,
                                                   mConfig->mName + ".devpts");

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

    utils::Channel channel;
    ContainerData data(*mConfig, channel);

    mConfig->mState = Container::State::STARTING;

    try {
        LOGD("Setting the guard process title");
        const std::string title = "[LXCPP] " + mConfig->mName + " " + mConfig->mRootPath;
        setProcTitle(title);
    } catch (std::exception &e) {
        // Ignore, this is optional
        LOGW("Failed to set the guard configuration: " << e.what());
    }

    containerPrepPreClone();

    mConfig->mInitPid = lxcpp::clone(startContainer,
                                     &data,
                                     mConfig->mNamespaces);

    containerPrepPostClone();

    // send continue sync to container once userns, netns, cgroups, etc, are configured
    channel.setLeft();
    channel.write(true);

    // wait for continue sync from the container
    channel.read<bool>();
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
    // TODO: Use SIGTERM instead of SIGKILL.
    utils::sendSignal(mConfig->mInitPid, SIGKILL);

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

    return status;
}

void Guard::onContTerminal(unsigned int i, int fd, cargo::ipc::epoll::Events events)
{
    if ((events & EPOLLIN) == EPOLLIN) {
        const size_t avail = IO_BUFFER_SIZE - mContToImplOffset[i];
        char *buf = mContToImpl[i].data() + mContToImplOffset[i];

        const ssize_t read = ::read(fd, buf, avail);

        if (read > 0) {
            mContToImplOffset[i] += read;

            if (mContToImplOffset[i]) {
                int oppositeFD = mImplSlaveFDs[i];
                mEventPoll.modifyFD(oppositeFD, EPOLLOUT);
            }
        }
    }

    if ((events & EPOLLOUT) == EPOLLOUT && mImplToContOffset[i]) {
        const ssize_t written = ::write(fd, mImplToCont[i].data(), mImplToContOffset[i]);

        if (written > 0) {
            ::memmove(mImplToCont[i].data(), mImplToCont[i].data() + written, mImplToContOffset[i] - written);
            mImplToContOffset[i] -= written;

            if (mImplToContOffset[i] == 0) {
                mEventPoll.modifyFD(fd, EPOLLIN);
            }
        }
    }
}

void Guard::onImplTerminal(unsigned int i, int fd, cargo::ipc::epoll::Events events)
{
    if ((events & EPOLLIN) == EPOLLIN) {
        const size_t avail = IO_BUFFER_SIZE - mImplToContOffset[i];
        char *buf = mImplToCont[i].data() + mImplToContOffset[i];

        const ssize_t read = ::read(fd, buf, avail);

        if (read > 0) {
            mImplToContOffset[i] += read;

            if (mImplToContOffset[i]) {
                int oppositeFD = mGuardPTYs.mPTYs[i].mMasterFD.value;
                mEventPoll.modifyFD(oppositeFD, EPOLLOUT);
            }
        }
    }

    if ((events & EPOLLOUT) == EPOLLOUT && mContToImplOffset[i]) {
        const ssize_t written = ::write(fd, mContToImpl[i].data(), mContToImplOffset[i]);

        if (written > 0) {
            ::memmove(mContToImpl[i].data(), mContToImpl[i].data() + written, mContToImplOffset[i] - written);
            mContToImplOffset[i] -= written;

            if (mContToImplOffset[i] == 0) {
                mEventPoll.modifyFD(fd, EPOLLIN);
            }
        }
    }
}

cargo::ipc::HandlerExitCode Guard::onResizeTerm(const cargo::ipc::PeerID,
                                                std::shared_ptr<api::Int> &data,
                                                cargo::ipc::MethodResult::Pointer result)
{
    int implFD = mImplSlaveFDs[data->value];
    int contFD = mGuardPTYs.mPTYs[data->value].mMasterFD.value;
    struct winsize wsz;

    utils::ioctl(implFD, TIOCGWINSZ, &wsz);
    utils::ioctl(contFD, TIOCSWINSZ, &wsz);

    result->setVoid();
    return cargo::ipc::HandlerExitCode::SUCCESS;
}


} // namespace lxcpp
