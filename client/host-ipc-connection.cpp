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

void HostIPCConnection::createSystem()
{
    mConnection.createSystem();
}

void HostIPCConnection::callGetZoneIds(vasum::api::ZoneIds& argOut)
{
    mConnection.call(vasum::api::host::METHOD_GET_ZONE_ID_LIST, argOut);
}

void HostIPCConnection::callGetActiveZoneId(vasum::api::ZoneId& argOut)
{
    mConnection.call(vasum::api::host::METHOD_GET_ACTIVE_ZONE_ID, argOut);
}

void HostIPCConnection::callSetActiveZone(const vasum::api::ZoneId& argIn)
{
    mConnection.call(vasum::api::host::METHOD_SET_ACTIVE_ZONE, argIn);
}

void HostIPCConnection::callGetZoneInfo(const vasum::api::ZoneId& argIn, vasum::api::ZoneInfoOut& argOut)
{
    mConnection.call(vasum::api::host::METHOD_GET_ZONE_INFO, argIn, argOut);
}

void HostIPCConnection::callSetNetdevAttrs(const vasum::api::SetNetDevAttrsIn& argIn)
{
    mConnection.call(vasum::api::host::METHOD_SET_NETDEV_ATTRS, argIn);
}

void HostIPCConnection::callGetNetdevAttrs(const vasum::api::GetNetDevAttrsIn& argIn, vasum::api::GetNetDevAttrs& argOut)
{
    mConnection.call(vasum::api::host::METHOD_GET_NETDEV_ATTRS, argIn, argOut);
}

void HostIPCConnection::callGetNetdevList(const vasum::api::ZoneId& argIn, vasum::api::NetDevList& argOut)
{
    mConnection.call(vasum::api::host::METHOD_GET_NETDEV_LIST, argIn, argOut);
}

void HostIPCConnection::callCreateNetdevVeth(const vasum::api::CreateNetDevVethIn& argIn)
{
    mConnection.call(vasum::api::host::METHOD_CREATE_NETDEV_VETH, argIn);
}

void HostIPCConnection::callCreateNetdevMacvlan(const vasum::api::CreateNetDevMacvlanIn& argIn)
{
    mConnection.call(vasum::api::host::METHOD_CREATE_NETDEV_MACVLAN, argIn);
}

void HostIPCConnection::callCreateNetdevPhys(const vasum::api::CreateNetDevPhysIn& argIn)
{
    mConnection.call(vasum::api::host::METHOD_CREATE_NETDEV_PHYS, argIn);
}

void HostIPCConnection::callDestroyNetdev(const vasum::api::DestroyNetDevIn& argIn)
{
    mConnection.call(vasum::api::host::METHOD_DESTROY_NETDEV, argIn);
}

void HostIPCConnection::callDeleteNetdevIpAddress(const vasum::api::DeleteNetdevIpAddressIn& argIn)
{
    mConnection.call(vasum::api::host::METHOD_DELETE_NETDEV_IP_ADDRESS, argIn);
}

void HostIPCConnection::callDeclareFile(const vasum::api::DeclareFileIn& argIn, vasum::api::Declaration& argOut)
{
    mConnection.call(vasum::api::host::METHOD_DECLARE_FILE, argIn, argOut);
}

void HostIPCConnection::callDeclareMount(const vasum::api::DeclareMountIn& argIn, vasum::api::Declaration& argOut)
{
    mConnection.call(vasum::api::host::METHOD_DECLARE_MOUNT, argIn, argOut);
}

void HostIPCConnection::callDeclareLink(const vasum::api::DeclareLinkIn& argIn, vasum::api::Declaration& argOut)
{
    mConnection.call(vasum::api::host::METHOD_DECLARE_LINK, argIn, argOut);
}

void HostIPCConnection::callGetDeclarations(const vasum::api::ZoneId& argIn, vasum::api::Declarations& argOut)
{
    mConnection.call(vasum::api::host::METHOD_GET_DECLARATIONS, argIn, argOut);
}

void HostIPCConnection::callRemoveDeclaration(const vasum::api::RemoveDeclarationIn& argIn)
{
    mConnection.call(vasum::api::host::METHOD_REMOVE_DECLARATION, argIn);
}

void HostIPCConnection::callCreateZone(const vasum::api::CreateZoneIn& argIn)
{
    mConnection.call(vasum::api::host::METHOD_CREATE_ZONE, argIn);
}

void HostIPCConnection::callDestroyZone(const vasum::api::ZoneId& argIn)
{
    mConnection.call(vasum::api::host::METHOD_DESTROY_ZONE, argIn);
}

void HostIPCConnection::callShutdownZone(const vasum::api::ZoneId& argIn)
{
    mConnection.call(vasum::api::host::METHOD_SHUTDOWN_ZONE, argIn);
}

void HostIPCConnection::callStartZone(const vasum::api::ZoneId& argIn)
{
    mConnection.call(vasum::api::host::METHOD_START_ZONE, argIn);
}

void HostIPCConnection::callLockZone(const vasum::api::ZoneId& argIn)
{
    mConnection.call(vasum::api::host::METHOD_LOCK_ZONE, argIn);
}

void HostIPCConnection::callUnlockZone(const vasum::api::ZoneId& argIn)
{
    mConnection.call(vasum::api::host::METHOD_UNLOCK_ZONE, argIn);
}

void HostIPCConnection::callGrantDevice(const vasum::api::GrantDeviceIn& argIn)
{
    mConnection.call(vasum::api::host::METHOD_GRANT_DEVICE, argIn);
}

void HostIPCConnection::callRevokeDevice(const vasum::api::RevokeDeviceIn& argIn)
{
    mConnection.call(vasum::api::host::METHOD_REVOKE_DEVICE, argIn);
}

void HostIPCConnection::callGetZoneDbuses(vasum::api::Dbuses& argOut)
{
    mConnection.call(vasum::api::host::METHOD_GET_ZONE_DBUSES, argOut);
}

HostIPCConnection::SubscriptionId
HostIPCConnection::subscribeZoneDbusState(const ZoneDbusStateCallback& callback)
{
    mConnection.subscribe<ZoneDbusStateCallback, vasum::api::DbusState>(
            vasum::api::host::SIGNAL_ZONE_DBUS_STATE, callback);
    return vasum::api::host::SIGNAL_ZONE_DBUS_STATE;
}

void HostIPCConnection::unsubscribe(const SubscriptionId& id)
{
    mConnection.unsubscribe(id);
}

} // namespace client
} // namespace vasum
