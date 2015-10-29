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
#include "lxcpp/guard/api.hpp"

#include "utils/channel.hpp"
#include "utils/signalfd.hpp"

#include "ipc/service.hpp"
#include "ipc/epoll/event-poll.hpp"


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

    ipc::epoll::EventPoll mEventPoll;
    ipc::PeerID mPeerID;
    utils::SignalFD mSignalFD;
    std::unique_ptr<ipc::Service> mService;

    std::shared_ptr<ContainerConfig> mConfig;

    // NOTE: Create a map if more async results needed
    ipc::MethodResult::Pointer mStopResult;

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
    void onConnection(const ipc::PeerID& peerID, const ipc::FileDescriptor);

    /**
     * Called each time a connection to host is lost.
     * It resets the peerID.
     *
     * @param peerID peerID of the connected client
     */
    void onDisconnection(const ipc::PeerID& peerID, const ipc::FileDescriptor);

    /**
     * Called when the init process exits.
     * Sends the exit status to the host.
     */
    void onInitExit();

    /**
     * Called when synchronizing configuration with the host
     *
     * @param data new config value
     */
    void onSetConfig(const ipc::PeerID, std::shared_ptr<ContainerConfig>& data, ipc::MethodResult::Pointer result);

    /**
    * Called when synchronizing configuration with the host
    *
    * @param result new config value
    */
    void onGetConfig(const ipc::PeerID, std::shared_ptr<api::Void>&, ipc::MethodResult::Pointer result);

    /**
     * Host -> Guard: Start init in a container described by the configuration
     */
    void onStart(const ipc::PeerID, std::shared_ptr<api::Void>&, ipc::MethodResult::Pointer result);

    /**
     * Host -> Guard: Stop the init process and return its exit status.
     * Returns the status asynchronously (outside onStop), when init dies.
     */
    void onStop(const ipc::PeerID, std::shared_ptr<api::Void>&, ipc::MethodResult::Pointer result);

};

} // namespace lxcpp

#endif // LXCPP_GUARD_HPP
