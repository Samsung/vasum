/*
 *  Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Mateusz Malicki <m.malicki2@samsung.com>
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License
 */

/**
 * @file
 * @author  Mateusz Malicki (m.malicki2@samsung.com)
 * @brief   HostIPCConnection class
 */


#ifndef SERVER_HOST_IPC_CONNECTION_HPP
#define SERVER_HOST_IPC_CONNECTION_HPP

#include "api/messages.hpp"
#include "ipc/epoll/event-poll.hpp"
#include "ipc/service.hpp"
#include "ipc-callback-wrapper.hpp"

namespace vasum {

class ZonesManager;

class HostIPCConnection {
public:
    template<typename ArgIn = const api::Void, typename ArgOut = api::Void>
    class Method {
    public:
        typedef typename IPCMethodWrapper<ArgIn, ArgOut>::type type;
    };
    template<typename ArgIn>
    class Signal {
    public:
        typedef typename IPCSignalWrapper<ArgIn>::type type;
    };

    HostIPCConnection(ipc::epoll::EventPoll& eventPoll, ZonesManager* zm);
    ~HostIPCConnection();

    void start();
    void stop(bool wait);
    void signalZoneConnectionState(const api::ConnectionState& connectionState);
    bool isRunning();

private:
    void setLockQueueCallback(const Method<api::Void>::type& callback);
    void setUnlockQueueCallback(const Method<api::Void>::type& callback);
    void setGetZoneIdsCallback(const Method<api::ZoneIds>::type& callback);
    void setGetZoneConnectionsCallback(const Method<api::Connections>::type& callback);
    void setGetActiveZoneIdCallback(const Method<api::ZoneId>::type& callback);
    void setGetZoneInfoCallback(const Method<const api::ZoneId, api::ZoneInfoOut>::type& callback);
    void setSetNetdevAttrsCallback(const Method<const api::SetNetDevAttrsIn>::type& callback);
    void setGetNetdevAttrsCallback(const Method<const api::GetNetDevAttrsIn, api::GetNetDevAttrs>::type& callback);
    void setGetNetdevListCallback(const Method<const api::ZoneId, api::NetDevList>::type& callback);
    void setCreateNetdevVethCallback(const Method<const api::CreateNetDevVethIn>::type& callback);
    void setCreateNetdevMacvlanCallback(const Method<const api::CreateNetDevMacvlanIn>::type& callback);
    void setCreateNetdevPhysCallback(const Method<const api::CreateNetDevPhysIn>::type& callback);
    void setDestroyNetdevCallback(const Method<const api::DestroyNetDevIn>::type& callback);
    void setDeleteNetdevIpAddressCallback(const Method<const api::DeleteNetdevIpAddressIn>::type& callback);
    void setDeclareFileCallback(const Method<const api::DeclareFileIn, api::Declaration>::type& callback);
    void setDeclareMountCallback(const Method<const api::DeclareMountIn, api::Declaration>::type& callback);
    void setDeclareLinkCallback(const Method<const api::DeclareLinkIn, api::Declaration>::type& callback);
    void setGetDeclarationsCallback(const Method<const api::ZoneId, api::Declarations>::type& callback);
    void setRemoveDeclarationCallback(const Method<const api::RemoveDeclarationIn>::type& callback);
    void setSetActiveZoneCallback(const Method<const api::ZoneId>::type& callback);
    void setCreateZoneCallback(const Method<const api::CreateZoneIn>::type& callback);
    void setDestroyZoneCallback(const Method<const api::ZoneId>::type& callback);
    void setShutdownZoneCallback(const Method<const api::ZoneId>::type& callback);
    void setStartZoneCallback(const Method<const api::ZoneId>::type& callback);
    void setLockZoneCallback(const Method<const api::ZoneId>::type& callback);
    void setUnlockZoneCallback(const Method<const api::ZoneId>::type& callback);
    void setGrantDeviceCallback(const Method<const api::GrantDeviceIn>::type& callback);
    void setRevokeDeviceCallback(const Method<const api::RevokeDeviceIn>::type& callback);
    void setSwitchToDefaultCallback(const Method<api::Void>::type& callback);
    void setCreateFileCallback(const Method<const api::CreateFileIn, api::CreateFileOut>::type& callback);

    std::unique_ptr<ipc::Service> mService;
    ZonesManager* mZonesManagerPtr;
};

} // namespace vasum

#endif // SERVER_HOST_IPC_CONNECTION_HPP
