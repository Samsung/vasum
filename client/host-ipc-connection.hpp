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

#ifndef VASUM_CLIENT_HOST_IPC_CONNECTION_HPP
#define VASUM_CLIENT_HOST_IPC_CONNECTION_HPP

#include <api/messages.hpp>
#include <epoll/thread-dispatcher.hpp>
#include <ipc/client.hpp>

#include <functional>

namespace vasum {
namespace client {

/**
 * HostIPCConnection is used for communication with the vasum's server from host through ipc
 */
class HostIPCConnection {
public:
    typedef unsigned int SubscriptionId;
    typedef std::function<void(const vasum::api::Notification&)> NotificationCallback;
    void createSystem();
    void create(const std::string& address);
    epoll::ThreadDispatcher& getDispatcher();

    void callGetZoneIds(vasum::api::ZoneIds& argOut);
    void callGetActiveZoneId(vasum::api::ZoneId& argOut);
    void callSetActiveZone(const vasum::api::ZoneId& argIn);
    void callGetZoneInfo(const vasum::api::ZoneId& argIn, vasum::api::ZoneInfoOut& argOut);
    void callSetNetdevAttrs(const vasum::api::SetNetDevAttrsIn& argIn);
    void callGetNetdevAttrs(const vasum::api::GetNetDevAttrsIn& argIn, vasum::api::GetNetDevAttrs& argOut);
    void callGetNetdevList(const vasum::api::ZoneId& argIn, vasum::api::NetDevList& argOut);
    void callCreateNetdevVeth(const vasum::api::CreateNetDevVethIn& argIn);
    void callCreateNetdevMacvlan(const vasum::api::CreateNetDevMacvlanIn& argIn);
    void callCreateNetdevPhys(const vasum::api::CreateNetDevPhysIn& argIn);
    void callDestroyNetdev(const vasum::api::DestroyNetDevIn& argIn);
    void callDeleteNetdevIpAddress(const vasum::api::DeleteNetdevIpAddressIn& argIn);
    void callDeclareFile(const vasum::api::DeclareFileIn& argIn, vasum::api::Declaration& argOut);
    void callDeclareMount(const vasum::api::DeclareMountIn& argIn, vasum::api::Declaration& argOut);
    void callDeclareLink(const vasum::api::DeclareLinkIn& argIn, vasum::api::Declaration& argOut);
    void callGetDeclarations(const vasum::api::ZoneId& argIn, vasum::api::Declarations& argOut);
    void callRemoveDeclaration(const vasum::api::RemoveDeclarationIn& argIn);
    void callCreateZone(const vasum::api::CreateZoneIn& argIn);
    void callDestroyZone(const vasum::api::ZoneId& argIn);
    void callShutdownZone(const vasum::api::ZoneId& argIn);
    void callStartZone(const vasum::api::ZoneId& argIn);
    void callLockZone(const vasum::api::ZoneId& argIn);
    void callUnlockZone(const vasum::api::ZoneId& argIn);
    void callGrantDevice(const vasum::api::GrantDeviceIn& argIn);
    void callRevokeDevice(const vasum::api::RevokeDeviceIn& argIn);
    void callGetZoneConnections(vasum::api::Connections& argOut);
    void callNotifyActiveZone(const vasum::api::NotifActiveZoneIn& argIn);
    void callFileMoveRequest(const vasum::api::FileMoveRequestIn& argIn,
                             vasum::api::FileMoveRequestStatus& argOut);
    void signalSwitchToDefault();
    SubscriptionId subscribeNotification(const NotificationCallback& callback);
    void unsubscribe(const SubscriptionId& id);

private:
    epoll::ThreadDispatcher mDispatcher;
    std::unique_ptr<ipc::Client> mClient;

    template<typename ArgIn, typename ArgOut>
    void call(const ipc::MethodID method, const ArgIn& argIn, ArgOut& argOut, int timeout = 5000) {
        auto out = mClient->callSync<ArgIn, ArgOut>(method, std::make_shared<ArgIn>(argIn), timeout);
        argOut = *out;
    }
};

} // namespace client
} // namespace vasum

#endif /* VASUM_CLIENT_HOST_IPC_CONNECTION_HPP */
