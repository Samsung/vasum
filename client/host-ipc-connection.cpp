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

#include <config.hpp>
#include "host-ipc-connection.hpp"
#include <api/messages.hpp>
#include <host-ipc-definitions.hpp>

namespace vasum {
namespace client {

namespace {
    const int TIMEOUT_INFINITE = -1;
} //namespace

void HostIPCConnection::createSystem()
{
    mClient.reset(new ipc::Client(mDispatcher.getPoll(), HOST_IPC_SOCKET));
    mClient->start();
}
ipc::epoll::ThreadDispatcher& HostIPCConnection::getDispatcher()
{
    return mDispatcher;
}

void HostIPCConnection::create(const std::string& address)
{
    mClient.reset(new ipc::Client(mDispatcher.getPoll(), address));
    mClient->start();
}

void HostIPCConnection::callGetZoneIds(api::ZoneIds& argOut)
{
    argOut = *mClient->callSync<api::Void, api::ZoneIds>(
       api::ipc::METHOD_GET_ZONE_ID_LIST,
       std::make_shared<api::Void>());
}

void HostIPCConnection::callGetActiveZoneId(api::ZoneId& argOut)
{
    argOut = *mClient->callSync<api::Void, api::ZoneId>(
       api::ipc::METHOD_GET_ACTIVE_ZONE_ID,
       std::make_shared<api::Void>());
}

void HostIPCConnection::callSetActiveZone(const api::ZoneId& argIn)
{
    mClient->callSync<api::ZoneId, api::Void>(
       api::ipc::METHOD_SET_ACTIVE_ZONE,
       std::make_shared<api::ZoneId>(argIn));
}

void HostIPCConnection::callGetZoneInfo(const api::ZoneId& argIn, api::ZoneInfoOut& argOut)
{
    argOut = *mClient->callSync<api::ZoneId, api::ZoneInfoOut>(
        api::ipc::METHOD_GET_ZONE_INFO,
        std::make_shared<api::ZoneId>(argIn));
}

void HostIPCConnection::callSetNetdevAttrs(const api::SetNetDevAttrsIn& argIn)
{
    mClient->callSync<api::SetNetDevAttrsIn, api::Void>(
       api::ipc::METHOD_SET_NETDEV_ATTRS,
       std::make_shared<api::SetNetDevAttrsIn>(argIn));
}

void HostIPCConnection::callGetNetdevAttrs(const api::GetNetDevAttrsIn& argIn, api::GetNetDevAttrs& argOut)
{
    argOut = *mClient->callSync<api::GetNetDevAttrsIn, api::GetNetDevAttrs>(
        api::ipc::METHOD_GET_NETDEV_ATTRS,
        std::make_shared<api::GetNetDevAttrsIn>(argIn));
}

void HostIPCConnection::callGetNetdevList(const api::ZoneId& argIn, api::NetDevList& argOut)
{
    argOut = *mClient->callSync<api::ZoneId, api::NetDevList>(
        api::ipc::METHOD_GET_NETDEV_LIST,
        std::make_shared<api::ZoneId>(argIn));
}

void HostIPCConnection::callCreateNetdevVeth(const api::CreateNetDevVethIn& argIn)
{
    mClient->callSync<api::CreateNetDevVethIn, api::Void>(
       api::ipc::METHOD_CREATE_NETDEV_VETH,
       std::make_shared<api::CreateNetDevVethIn>(argIn));
}

void HostIPCConnection::callCreateNetdevMacvlan(const api::CreateNetDevMacvlanIn& argIn)
{
    mClient->callSync<api::CreateNetDevMacvlanIn, api::Void>(
       api::ipc::METHOD_CREATE_NETDEV_MACVLAN,
       std::make_shared<api::CreateNetDevMacvlanIn>(argIn));
}

void HostIPCConnection::callCreateNetdevPhys(const api::CreateNetDevPhysIn& argIn)
{
    mClient->callSync<api::CreateNetDevPhysIn, api::Void>(
       api::ipc::METHOD_CREATE_NETDEV_PHYS,
       std::make_shared<api::CreateNetDevPhysIn>(argIn));
}

void HostIPCConnection::callDestroyNetdev(const api::DestroyNetDevIn& argIn)
{
    mClient->callSync<api::DestroyNetDevIn, api::Void>(
       api::ipc::METHOD_DESTROY_NETDEV,
       std::make_shared<api::DestroyNetDevIn>(argIn));
}

void HostIPCConnection::callDeleteNetdevIpAddress(const api::DeleteNetdevIpAddressIn& argIn)
{
    mClient->callSync<api::DeleteNetdevIpAddressIn, api::Void>(
       api::ipc::METHOD_DELETE_NETDEV_IP_ADDRESS,
       std::make_shared<api::DeleteNetdevIpAddressIn>(argIn));
}

void HostIPCConnection::callDeclareFile(const api::DeclareFileIn& argIn, api::Declaration& argOut)
{
    argOut = *mClient->callSync<api::DeclareFileIn, api::Declaration>(
        api::ipc::METHOD_DECLARE_FILE,
        std::make_shared<api::DeclareFileIn>(argIn));
}

void HostIPCConnection::callDeclareMount(const api::DeclareMountIn& argIn, api::Declaration& argOut)
{
    argOut = *mClient->callSync<api::DeclareMountIn, api::Declaration>(
        api::ipc::METHOD_DECLARE_MOUNT,
        std::make_shared<api::DeclareMountIn>(argIn));
}

void HostIPCConnection::callDeclareLink(const api::DeclareLinkIn& argIn, api::Declaration& argOut)
{
    argOut = *mClient->callSync<api::DeclareLinkIn, api::Declaration>(
        api::ipc::METHOD_DECLARE_LINK,
        std::make_shared<api::DeclareLinkIn>(argIn));
}

void HostIPCConnection::callGetDeclarations(const api::ZoneId& argIn, api::Declarations& argOut)
{
    argOut = *mClient->callSync<api::ZoneId, api::Declarations>(
        api::ipc::METHOD_GET_DECLARATIONS,
        std::make_shared<api::ZoneId>(argIn));
}

void HostIPCConnection::callRemoveDeclaration(const api::RemoveDeclarationIn& argIn)
{
    mClient->callSync<api::RemoveDeclarationIn, api::Void>(
        api::ipc::METHOD_REMOVE_DECLARATION,
        std::make_shared<api::RemoveDeclarationIn>(argIn));
}

void HostIPCConnection::callCreateZone(const api::CreateZoneIn& argIn)
{
    mClient->callSync<api::CreateZoneIn, api::Void>(
        api::ipc::METHOD_CREATE_ZONE,
        std::make_shared<api::CreateZoneIn>(argIn),
        TIMEOUT_INFINITE);
}

void HostIPCConnection::callDestroyZone(const api::ZoneId& argIn)
{
    mClient->callSync<api::ZoneId, api::Void>(
       api::ipc::METHOD_DESTROY_ZONE,
       std::make_shared<api::ZoneId>(argIn),
       TIMEOUT_INFINITE);
}

void HostIPCConnection::callShutdownZone(const api::ZoneId& argIn)
{
    mClient->callSync<api::ZoneId, api::Void>(
        api::ipc::METHOD_SHUTDOWN_ZONE,
        std::make_shared<api::ZoneId>(argIn),
        TIMEOUT_INFINITE);
}

void HostIPCConnection::callStartZone(const api::ZoneId& argIn)
{
    mClient->callSync<api::ZoneId, api::Void>(
        api::ipc::METHOD_START_ZONE,
        std::make_shared<api::ZoneId>(argIn),
        TIMEOUT_INFINITE);
}

void HostIPCConnection::callLockZone(const api::ZoneId& argIn)
{
    mClient->callSync<api::ZoneId, api::Void>(
        api::ipc::METHOD_LOCK_ZONE,
        std::make_shared<api::ZoneId>(argIn),
        TIMEOUT_INFINITE);
}

void HostIPCConnection::callUnlockZone(const api::ZoneId& argIn)
{
    mClient->callSync<api::ZoneId, api::Void>(
        api::ipc::METHOD_UNLOCK_ZONE,
        std::make_shared<api::ZoneId>(argIn));
}

void HostIPCConnection::callGrantDevice(const api::GrantDeviceIn& argIn)
{
    mClient->callSync<api::GrantDeviceIn, api::Void>(
        api::ipc::METHOD_GRANT_DEVICE,
        std::make_shared<api::GrantDeviceIn>(argIn));
}

void HostIPCConnection::callRevokeDevice(const api::RevokeDeviceIn& argIn)
{
    mClient->callSync<api::RevokeDeviceIn, api::Void>(
        api::ipc::METHOD_REVOKE_DEVICE,
        std::make_shared<api::RevokeDeviceIn>(argIn));
}

void HostIPCConnection::callNotifyActiveZone(const api::NotifActiveZoneIn& argIn)
{
    mClient->callSync<api::NotifActiveZoneIn, api::Void>(
        api::ipc::METHOD_NOTIFY_ACTIVE_ZONE,
        std::make_shared<api::NotifActiveZoneIn>(argIn));
}

void HostIPCConnection::callFileMoveRequest(const api::FileMoveRequestIn& argIn,
                                            api::FileMoveRequestStatus& argOut)
{
    argOut = *mClient->callSync<api::FileMoveRequestIn, api::FileMoveRequestStatus>(
        api::ipc::METHOD_FILE_MOVE_REQUEST,
        std::make_shared<api::FileMoveRequestIn>(argIn));
}

void HostIPCConnection::signalSwitchToDefault()
{

    mClient->signal(api::ipc::SIGNAL_SWITCH_TO_DEFAULT,
                    std::make_shared<api::Void>());
}

HostIPCConnection::SubscriptionId
HostIPCConnection::subscribeNotification(const NotificationCallback& callback)
{
    auto callbackWrapper = [callback] (const ipc::PeerID,
                                       std::shared_ptr<api::Notification>& data) {
        callback(*data);
    };
    mClient->setSignalHandler<api::Notification>(api::ipc::SIGNAL_NOTIFICATION, callbackWrapper);
    return api::ipc::SIGNAL_NOTIFICATION;
}

void HostIPCConnection::unsubscribe(const SubscriptionId& id)
{
    mClient->removeMethod(id);
}

} // namespace client
} // namespace vasum
