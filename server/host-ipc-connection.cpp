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

namespace {

const std::string SOCKET_PATH = HOST_IPC_SOCKET;

} // namespace


HostIPCConnection::HostIPCConnection()
{
    LOGT("Connecting to host IPC socket");
    mService.reset(new ipc::Service(mDispatcher.getPoll(), SOCKET_PATH));

    LOGT("Starting IPC");
    mService->start();
    LOGD("Connected");
}

HostIPCConnection::~HostIPCConnection()
{
}

void HostIPCConnection::setGetZoneConnectionsCallback(const Callback<api::Connections>::type& callback)
{
    typedef Callback<api::Connections> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::host::METHOD_GET_ZONE_CONNECTIONS,
        Callback::getCallbackWrapper(callback));

}

void HostIPCConnection::setGetZoneIdsCallback(const Callback<api::ZoneIds>::type& callback)
{
    typedef Callback<api::ZoneIds> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::host::METHOD_GET_ZONE_ID_LIST,
        Callback::getCallbackWrapper(callback));

}

void HostIPCConnection::setGetActiveZoneIdCallback(const Callback<api::ZoneId>::type& callback)
{
    typedef Callback<api::ZoneId> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::host::METHOD_GET_ACTIVE_ZONE_ID,
        Callback::getCallbackWrapper(callback));

}

void HostIPCConnection::setGetZoneInfoCallback(const Callback<const api::ZoneId, api::ZoneInfoOut>::type& callback)
{
    typedef Callback<const api::ZoneId, api::ZoneInfoOut> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::host::METHOD_GET_ZONE_INFO,
        Callback::getCallbackWrapper(callback));
}

void HostIPCConnection::setSetNetdevAttrsCallback(const Callback<const api::SetNetDevAttrsIn>::type& callback)
{
    typedef Callback<const api::SetNetDevAttrsIn> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::host::METHOD_SET_NETDEV_ATTRS,
        Callback::getCallbackWrapper(callback));
}

void HostIPCConnection::setGetNetdevAttrsCallback(const Callback<const api::GetNetDevAttrsIn, api::GetNetDevAttrs>::type& callback)
{
    typedef Callback<const api::GetNetDevAttrsIn, api::GetNetDevAttrs> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::host::METHOD_GET_NETDEV_ATTRS,
        Callback::getCallbackWrapper(callback));
}

void HostIPCConnection::setGetNetdevListCallback(const Callback<const api::ZoneId, api::NetDevList>::type& callback)
{
    typedef Callback<const api::ZoneId, api::NetDevList> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::host::METHOD_GET_NETDEV_LIST,
        Callback::getCallbackWrapper(callback));
}

void HostIPCConnection::setCreateNetdevVethCallback(const Callback<const api::CreateNetDevVethIn>::type& callback)
{
    typedef Callback<const api::CreateNetDevVethIn> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::host::METHOD_CREATE_NETDEV_VETH,
        Callback::getCallbackWrapper(callback));
}

void HostIPCConnection::setCreateNetdevMacvlanCallback(const Callback<const api::CreateNetDevMacvlanIn>::type& callback)
{
    typedef Callback<const api::CreateNetDevMacvlanIn> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::host::METHOD_CREATE_NETDEV_MACVLAN,
        Callback::getCallbackWrapper(callback));
}

void HostIPCConnection::setCreateNetdevPhysCallback(const Callback<const api::CreateNetDevPhysIn>::type& callback)
{
    typedef Callback<const api::CreateNetDevPhysIn> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::host::METHOD_CREATE_NETDEV_PHYS,
        Callback::getCallbackWrapper(callback));
}

void HostIPCConnection::setDestroyNetdevCallback(const Callback<const api::DestroyNetDevIn>::type& callback)
{
    typedef Callback<const api::DestroyNetDevIn> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::host::METHOD_DESTROY_NETDEV,
        Callback::getCallbackWrapper(callback));
}

void HostIPCConnection::setDeleteNetdevIpAddressCallback(const Callback<const api::DeleteNetdevIpAddressIn>::type& callback)
{
    typedef Callback<const api::DeleteNetdevIpAddressIn> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::host::METHOD_DELETE_NETDEV_IP_ADDRESS,
        Callback::getCallbackWrapper(callback));
}

void HostIPCConnection::setDeclareFileCallback(const Callback<const api::DeclareFileIn, api::Declaration>::type& callback)
{
    typedef Callback<const api::DeclareFileIn, api::Declaration> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::host::METHOD_DECLARE_FILE,
        Callback::getCallbackWrapper(callback));
}

void HostIPCConnection::setDeclareMountCallback(const Callback<const api::DeclareMountIn, api::Declaration>::type& callback)
{
    typedef Callback<const api::DeclareMountIn, api::Declaration> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::host::METHOD_DECLARE_MOUNT,
        Callback::getCallbackWrapper(callback));
}

void HostIPCConnection::setDeclareLinkCallback(const Callback<const api::DeclareLinkIn, api::Declaration>::type& callback)
{
    typedef Callback<const api::DeclareLinkIn, api::Declaration> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::host::METHOD_DECLARE_LINK,
        Callback::getCallbackWrapper(callback));
}

void HostIPCConnection::setGetDeclarationsCallback(const Callback<const api::ZoneId, api::Declarations>::type& callback)
{
    typedef Callback<const api::ZoneId, api::Declarations> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::host::METHOD_GET_DECLARATIONS,
        Callback::getCallbackWrapper(callback));
}

void HostIPCConnection::setRemoveDeclarationCallback(const Callback<const api::RemoveDeclarationIn>::type& callback)
{
    typedef Callback<const api::RemoveDeclarationIn> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::host::METHOD_REMOVE_DECLARATION,
        Callback::getCallbackWrapper(callback));
}

void HostIPCConnection::setSetActiveZoneCallback(const Callback<const api::ZoneId>::type& callback)
{
    typedef Callback<const api::ZoneId> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::host::METHOD_SET_ACTIVE_ZONE,
        Callback::getCallbackWrapper(callback));
}

void HostIPCConnection::setCreateZoneCallback(const Callback<const api::CreateZoneIn>::type& callback)
{
    typedef Callback<const api::CreateZoneIn> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::host::METHOD_CREATE_ZONE,
        Callback::getCallbackWrapper(callback));
}

void HostIPCConnection::setDestroyZoneCallback(const Callback<const api::ZoneId>::type& callback)
{
    typedef Callback<const api::ZoneId> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::host::METHOD_DESTROY_ZONE,
        Callback::getCallbackWrapper(callback));
}

void HostIPCConnection::setShutdownZoneCallback(const Callback<const api::ZoneId>::type& callback)
{
    typedef Callback<const api::ZoneId> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::host::METHOD_SHUTDOWN_ZONE,
        Callback::getCallbackWrapper(callback));
}

void HostIPCConnection::setStartZoneCallback(const Callback<const api::ZoneId>::type& callback)
{
    typedef Callback<const api::ZoneId> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::host::METHOD_START_ZONE,
        Callback::getCallbackWrapper(callback));
}

void HostIPCConnection::setLockZoneCallback(const Callback<const api::ZoneId>::type& callback)
{
    typedef Callback<const api::ZoneId> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::host::METHOD_LOCK_ZONE,
        Callback::getCallbackWrapper(callback));
}

void HostIPCConnection::setUnlockZoneCallback(const Callback<const api::ZoneId>::type& callback)
{
    typedef Callback<const api::ZoneId> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::host::METHOD_UNLOCK_ZONE,
        Callback::getCallbackWrapper(callback));
}

void HostIPCConnection::setGrantDeviceCallback(const Callback<const api::GrantDeviceIn>::type& callback)
{
    typedef Callback<const api::GrantDeviceIn> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::host::METHOD_GRANT_DEVICE,
        Callback::getCallbackWrapper(callback));
}

void HostIPCConnection::setRevokeDeviceCallback(const Callback<const api::RevokeDeviceIn>::type& callback)
{
    typedef Callback<const api::RevokeDeviceIn> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::host::METHOD_REVOKE_DEVICE,
        Callback::getCallbackWrapper(callback));
}

void HostIPCConnection::signalZoneConnectionState(const api::ConnectionState& connectionState)
{
   mService->signal(api::host::SIGNAL_ZONE_CONNECTION_STATE,
                    std::make_shared<api::ConnectionState>(connectionState));
}

} // namespace vasum
