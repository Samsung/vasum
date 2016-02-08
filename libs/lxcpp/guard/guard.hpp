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
 * @brief   LXCPP guard process header
 */

#ifndef LXCPP_GUARD_GUARD_HPP
#define LXCPP_GUARD_GUARD_HPP

#include "lxcpp/container-config.hpp"
#include "lxcpp/pty-config.hpp"
#include "lxcpp/guard/api.hpp"

#include "utils/channel.hpp"
#include "utils/signalfd.hpp"

#include "cargo-ipc/service.hpp"
#include "cargo-ipc/epoll/event-poll.hpp"


namespace lxcpp {

/**
 * Guard process used for container's init configuration and control.
 *
 * Guard is a one thread process with a polling loop running till the init process exits.
 * Therefore no synchronization is needed inside the callbacks.
 *
 * All actions are triggered by the host or init process.
 */
class Guard {
public:
    Guard(const std::string& socketPath);
    ~Guard();

    int execute();

private:
    struct ContainerData {
        ContainerConfig &mConfig;
        utils::Channel &mChannel;

        ContainerData(ContainerConfig &config, utils::Channel &channel)
            : mConfig(config), mChannel(channel) {}
    };

    cargo::ipc::epoll::EventPoll mEventPoll;
    cargo::ipc::PeerID mPeerID;
    utils::SignalFD mSignalFD;
    std::unique_ptr<cargo::ipc::Service> mService;

    std::shared_ptr<ContainerConfig> mConfig;

    std::vector<int> mImplSlaveFDs;
    PTYsConfig mGuardPTYs;

    static const int IO_BUFFER_SIZE = 1024;
    std::vector<std::array<char, IO_BUFFER_SIZE>> mContToImpl;
    std::vector<std::array<char, IO_BUFFER_SIZE>> mImplToCont;
    std::vector<int> mContToImplOffset;
    std::vector<int> mImplToContOffset;

    /**
     * Container preparation part 1.
     *
     * All things that can and/or have to be done before the clone.
     * They run in the guard's context.
     * Some of the things made here might need to be cleaned up/reversed.
     */
    void containerPrepPreClone();

    /**
     * Container preparation part 2.
     *
     * All the things that has to be done immediately after clone.
     * Things that need to be done from the guard's context, but require
     * the container's init process to be already created.
     * Most of the things will not have to be reversed. They'll disappear
     * together with the container's init process and its namespaces.
     */
    void containerPrepPostClone();

    /**
     * Container preparation part 3.
     *
     * Things to do inside the container's process.
     *
     * @param config  container's config
     */
    static void containerPrepInClone(ContainerConfig &config);

    /**
     * Container cleanup.
     *
     * Cleanups and commands' reverses. Mostly from the things that has
     * been configured inside the containerPrepPreClone() function.
     * This happens after the container's init and the guard has died.
     */
    void containerCleanup();

    /**
     * Setups the init process and executes the init.
     */
    static int startContainer(void *data);

    /**
     * Called each time a new connection is opened to the Guard's socket.
     * There should be only one peer. It's ID is saved for future reuse.
     *
     * @param peerID peerID of the connected client
     */
    void onConnection(const cargo::ipc::PeerID& peerID, const cargo::ipc::FileDescriptor);

    /**
     * Called each time a connection to host is lost.
     * It resets the peerID.
     *
     * @param peerID peerID of the connected client
     */
    void onDisconnection(const cargo::ipc::PeerID& peerID, const cargo::ipc::FileDescriptor);

    /**
     * Called when the init process exits.
     * Sends the exit status to the host.
     *
     * @param sigInfo info about the received signal
     */
    void onInitExit(struct ::signalfd_siginfo& sigInfo);

    /**
     * Called when synchronizing configuration with the host
     *
     * @param data new config value
     */
    cargo::ipc::HandlerExitCode onSetConfig(const cargo::ipc::PeerID,
                                            std::shared_ptr<ContainerConfig>& data,
                                            cargo::ipc::MethodResult::Pointer result);

    /**
    * Called when synchronizing configuration with the host
    *
    * @param result new config value
    */
    cargo::ipc::HandlerExitCode onGetConfig(const cargo::ipc::PeerID,
                                            std::shared_ptr<api::Void>&,
                                            cargo::ipc::MethodResult::Pointer result);

    /**
     * Host -> Guard: Start init in a container described by the configuration
     */
    cargo::ipc::HandlerExitCode onStart(const cargo::ipc::PeerID,
                                        std::shared_ptr<api::Void>&,
                                        cargo::ipc::MethodResult::Pointer result);

    /**
     * Host -> Guard: Stop the init process and return its exit status.
     * Returns the status asynchronously (outside onStop), when init dies.
     */
    cargo::ipc::HandlerExitCode onStop(const cargo::ipc::PeerID,
                                       std::shared_ptr<api::Void>&,
                                       cargo::ipc::MethodResult::Pointer result);

    cargo::ipc::HandlerExitCode onResizeTerm(const cargo::ipc::PeerID,
                                             std::shared_ptr<api::Int> &data,
                                             cargo::ipc::MethodResult::Pointer result);

    /**
     * A callback for container terminal PTY master
     */
    void onContTerminal(unsigned int i, int fd, cargo::ipc::epoll::Events events);

    /**
     * A callback for implementation (container-impl.cpp) terminal PTY slave
     */
    void onImplTerminal(unsigned int i, int fd, cargo::ipc::epoll::Events events);
};

} // namespace lxcpp

#endif // LXCPP_GUARD_GUARD_HPP
