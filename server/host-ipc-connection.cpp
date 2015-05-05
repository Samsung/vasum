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

#include "config.hpp"

#include "host-ipc-connection.hpp"
#include "host-ipc-definitions.hpp"
#include "exception.hpp"
#include "logger/logger.hpp"

namespace vasum {

HostIPCConnection::HostIPCConnection()
{
    LOGT("Connecting to host IPC socket");
    mService.reset(new ipc::Service(mDispatcher.getPoll(), HOST_IPC_SOCKET));

    LOGT("Starting IPC");
    mService->start();
    LOGD("Connected");
}

HostIPCConnection::~HostIPCConnection()
{
}

void HostIPCConnection::setGetZoneIdsCallback(const Method<api::ZoneIds>::type& callback)
{
    typedef IPCMethodWrapper<api::ZoneIds> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::ipc::METHOD_GET_ZONE_ID_LIST,
        Callback::getWrapper(callback));

}

void HostIPCConnection::setGetActiveZoneIdCallback(const Method<api::ZoneId>::type& callback)
{
    typedef IPCMethodWrapper<api::ZoneId> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::ipc::METHOD_GET_ACTIVE_ZONE_ID,
        Callback::getWrapper(callback));

}

void HostIPCConnection::setGetZoneInfoCallback(const Method<const api::ZoneId, api::ZoneInfoOut>::type& callback)
{
    typedef IPCMethodWrapper<const api::ZoneId, api::ZoneInfoOut> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::ipc::METHOD_GET_ZONE_INFO,
        Callback::getWrapper(callback));
}

void HostIPCConnection::setSetNetdevAttrsCallback(const Method<const api::SetNetDevAttrsIn>::type& callback)
{
    typedef IPCMethodWrapper<const api::SetNetDevAttrsIn> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::ipc::METHOD_SET_NETDEV_ATTRS,
        Callback::getWrapper(callback));
}

void HostIPCConnection::setGetNetdevAttrsCallback(const Method<const api::GetNetDevAttrsIn, api::GetNetDevAttrs>::type& callback)
{
    typedef IPCMethodWrapper<const api::GetNetDevAttrsIn, api::GetNetDevAttrs> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::ipc::METHOD_GET_NETDEV_ATTRS,
        Callback::getWrapper(callback));
}

void HostIPCConnection::setGetNetdevListCallback(const Method<const api::ZoneId, api::NetDevList>::type& callback)
{
    typedef IPCMethodWrapper<const api::ZoneId, api::NetDevList> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::ipc::METHOD_GET_NETDEV_LIST,
        Callback::getWrapper(callback));
}

void HostIPCConnection::setCreateNetdevVethCallback(const Method<const api::CreateNetDevVethIn>::type& callback)
{
    typedef IPCMethodWrapper<const api::CreateNetDevVethIn> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::ipc::METHOD_CREATE_NETDEV_VETH,
        Callback::getWrapper(callback));
}

void HostIPCConnection::setCreateNetdevMacvlanCallback(const Method<const api::CreateNetDevMacvlanIn>::type& callback)
{
    typedef IPCMethodWrapper<const api::CreateNetDevMacvlanIn> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::ipc::METHOD_CREATE_NETDEV_MACVLAN,
        Callback::getWrapper(callback));
}

void HostIPCConnection::setCreateNetdevPhysCallback(const Method<const api::CreateNetDevPhysIn>::type& callback)
{
    typedef IPCMethodWrapper<const api::CreateNetDevPhysIn> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::ipc::METHOD_CREATE_NETDEV_PHYS,
        Callback::getWrapper(callback));
}

void HostIPCConnection::setDestroyNetdevCallback(const Method<const api::DestroyNetDevIn>::type& callback)
{
    typedef IPCMethodWrapper<const api::DestroyNetDevIn> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::ipc::METHOD_DESTROY_NETDEV,
        Callback::getWrapper(callback));
}

void HostIPCConnection::setDeleteNetdevIpAddressCallback(const Method<const api::DeleteNetdevIpAddressIn>::type& callback)
{
    typedef IPCMethodWrapper<const api::DeleteNetdevIpAddressIn> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::ipc::METHOD_DELETE_NETDEV_IP_ADDRESS,
        Callback::getWrapper(callback));
}

void HostIPCConnection::setDeclareFileCallback(const Method<const api::DeclareFileIn, api::Declaration>::type& callback)
{
    typedef IPCMethodWrapper<const api::DeclareFileIn, api::Declaration> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::ipc::METHOD_DECLARE_FILE,
        Callback::getWrapper(callback));
}

void HostIPCConnection::setDeclareMountCallback(const Method<const api::DeclareMountIn, api::Declaration>::type& callback)
{
    typedef IPCMethodWrapper<const api::DeclareMountIn, api::Declaration> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::ipc::METHOD_DECLARE_MOUNT,
        Callback::getWrapper(callback));
}

void HostIPCConnection::setDeclareLinkCallback(const Method<const api::DeclareLinkIn, api::Declaration>::type& callback)
{
    typedef IPCMethodWrapper<const api::DeclareLinkIn, api::Declaration> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::ipc::METHOD_DECLARE_LINK,
        Callback::getWrapper(callback));
}

void HostIPCConnection::setGetDeclarationsCallback(const Method<const api::ZoneId, api::Declarations>::type& callback)
{
    typedef IPCMethodWrapper<const api::ZoneId, api::Declarations> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::ipc::METHOD_GET_DECLARATIONS,
        Callback::getWrapper(callback));
}

void HostIPCConnection::setRemoveDeclarationCallback(const Method<const api::RemoveDeclarationIn>::type& callback)
{
    typedef IPCMethodWrapper<const api::RemoveDeclarationIn> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::ipc::METHOD_REMOVE_DECLARATION,
        Callback::getWrapper(callback));
}

void HostIPCConnection::setSetActiveZoneCallback(const Method<const api::ZoneId>::type& callback)
{
    typedef IPCMethodWrapper<const api::ZoneId> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::ipc::METHOD_SET_ACTIVE_ZONE,
        Callback::getWrapper(callback));
}

void HostIPCConnection::setCreateZoneCallback(const Method<const api::CreateZoneIn>::type& callback)
{
    typedef IPCMethodWrapper<const api::CreateZoneIn> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::ipc::METHOD_CREATE_ZONE,
        Callback::getWrapper(callback));
}

void HostIPCConnection::setDestroyZoneCallback(const Method<const api::ZoneId>::type& callback)
{
    typedef IPCMethodWrapper<const api::ZoneId> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::ipc::METHOD_DESTROY_ZONE,
        Callback::getWrapper(callback));
}

void HostIPCConnection::setShutdownZoneCallback(const Method<const api::ZoneId>::type& callback)
{
    typedef IPCMethodWrapper<const api::ZoneId> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::ipc::METHOD_SHUTDOWN_ZONE,
        Callback::getWrapper(callback));
}

void HostIPCConnection::setStartZoneCallback(const Method<const api::ZoneId>::type& callback)
{
    typedef IPCMethodWrapper<const api::ZoneId> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::ipc::METHOD_START_ZONE,
        Callback::getWrapper(callback));
}

void HostIPCConnection::setLockZoneCallback(const Method<const api::ZoneId>::type& callback)
{
    typedef IPCMethodWrapper<const api::ZoneId> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::ipc::METHOD_LOCK_ZONE,
        Callback::getWrapper(callback));
}

void HostIPCConnection::setUnlockZoneCallback(const Method<const api::ZoneId>::type& callback)
{
    typedef IPCMethodWrapper<const api::ZoneId> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::ipc::METHOD_UNLOCK_ZONE,
        Callback::getWrapper(callback));
}

void HostIPCConnection::setGrantDeviceCallback(const Method<const api::GrantDeviceIn>::type& callback)
{
    typedef IPCMethodWrapper<const api::GrantDeviceIn> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::ipc::METHOD_GRANT_DEVICE,
        Callback::getWrapper(callback));
}

void HostIPCConnection::setRevokeDeviceCallback(const Method<const api::RevokeDeviceIn>::type& callback)
{
    typedef IPCMethodWrapper<const api::RevokeDeviceIn> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::ipc::METHOD_REVOKE_DEVICE,
        Callback::getWrapper(callback));
}

void HostIPCConnection::setNotifyActiveZoneCallback(
    const Method<const vasum::api::NotifActiveZoneIn>::type& callback)
{
    typedef IPCMethodWrapper<const api::NotifActiveZoneIn> Method;
    mService->setMethodHandler<Method::out, Method::in>(
        api::ipc::METHOD_NOTIFY_ACTIVE_ZONE,
        Method::getWrapper(callback));
}

void HostIPCConnection::setSwitchToDefaultCallback(const Signal<const api::Void>::type& callback)
{
    typedef IPCSignalWrapper<const api::Void> Signal;
    mService->setSignalHandler<Signal::in>(
        api::ipc::SIGNAL_SWITCH_TO_DEFAULT,
        Signal::getWrapper(callback));
}

void HostIPCConnection::setFileMoveCallback(const Method<const api::FileMoveRequestIn,
                                            api::FileMoveRequestStatus>::type& callback)
{
    typedef IPCMethodWrapper<const api::FileMoveRequestIn, api::FileMoveRequestStatus> Method;
    mService->setMethodHandler<Method::out, Method::in>(
        api::ipc::METHOD_FILE_MOVE_REQUEST,
        Method::getWrapper(callback));
}

void HostIPCConnection::sendNotification(const api::Notification& notification)
{
    mService->signal(api::ipc::SIGNAL_NOTIFICATION,
                     std::make_shared<api::Notification>(notification));
}

} // namespace vasum
