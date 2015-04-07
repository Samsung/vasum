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
 * @brief   Host client class
 */


#include <config.hpp>
#include "host-dbus-connection.hpp"
#include <api/messages.hpp>
#include <host-dbus-definitions.hpp>

namespace vasum {
namespace client {

HostDbusConnection::HostDbusConnection()
    : mConnection(vasum::api::host::DEFINITION,
                  vasum::api::host::BUS_NAME,
                  vasum::api::host::OBJECT_PATH,
                  vasum::api::host::INTERFACE)
{
}

void HostDbusConnection::create(const std::shared_ptr<dbus::DbusConnection>& connection)
{
    mConnection.create(connection);
}

void HostDbusConnection::callGetZoneIds(vasum::api::ZoneIds& argOut)
{
    mConnection.call(vasum::api::host::METHOD_GET_ZONE_ID_LIST, argOut);
}

void HostDbusConnection::callGetActiveZoneId(vasum::api::ZoneId& argOut)
{
    mConnection.call(vasum::api::host::METHOD_GET_ACTIVE_ZONE_ID, argOut);
}

void HostDbusConnection::callSetActiveZone(const vasum::api::ZoneId& argIn)
{
    mConnection.call(vasum::api::host::METHOD_SET_ACTIVE_ZONE, argIn);
}

void HostDbusConnection::callGetZoneInfo(const vasum::api::ZoneId& argIn, vasum::api::ZoneInfoOut& argOut)
{
    mConnection.call(vasum::api::host::METHOD_GET_ZONE_INFO, argIn, argOut);
}

void HostDbusConnection::callSetNetdevAttrs(const vasum::api::SetNetDevAttrsIn& argIn)
{
    mConnection.call(vasum::api::host::METHOD_SET_NETDEV_ATTRS, argIn);
}

void HostDbusConnection::callGetNetdevAttrs(const vasum::api::GetNetDevAttrsIn& argIn, vasum::api::GetNetDevAttrs& argOut)
{
    mConnection.call(vasum::api::host::METHOD_GET_NETDEV_ATTRS, argIn, argOut);
}

void HostDbusConnection::callGetNetdevList(const vasum::api::ZoneId& argIn, vasum::api::NetDevList& argOut)
{
    mConnection.call(vasum::api::host::METHOD_GET_NETDEV_LIST, argIn, argOut);
}

void HostDbusConnection::callCreateNetdevVeth(const vasum::api::CreateNetDevVethIn& argIn)
{
    mConnection.call(vasum::api::host::METHOD_CREATE_NETDEV_VETH, argIn);
}

void HostDbusConnection::callCreateNetdevMacvlan(const vasum::api::CreateNetDevMacvlanIn& argIn)
{
    mConnection.call(vasum::api::host::METHOD_CREATE_NETDEV_MACVLAN, argIn);
}

void HostDbusConnection::callCreateNetdevPhys(const vasum::api::CreateNetDevPhysIn& argIn)
{
    mConnection.call(vasum::api::host::METHOD_CREATE_NETDEV_PHYS, argIn);
}

void HostDbusConnection::callDestroyNetdev(const vasum::api::DestroyNetDevIn& argIn)
{
    mConnection.call(vasum::api::host::METHOD_DESTROY_NETDEV, argIn);
}

void HostDbusConnection::callDeleteNetdevIpAddress(const vasum::api::DeleteNetdevIpAddressIn& argIn)
{
    mConnection.call(vasum::api::host::METHOD_DELETE_NETDEV_IP_ADDRESS, argIn);
}

void HostDbusConnection::callDeclareFile(const vasum::api::DeclareFileIn& argIn, vasum::api::Declaration& argOut)
{
    mConnection.call(vasum::api::host::METHOD_DECLARE_FILE, argIn, argOut);
}

void HostDbusConnection::callDeclareMount(const vasum::api::DeclareMountIn& argIn, vasum::api::Declaration& argOut)
{
    mConnection.call(vasum::api::host::METHOD_DECLARE_MOUNT, argIn, argOut);
}

void HostDbusConnection::callDeclareLink(const vasum::api::DeclareLinkIn& argIn, vasum::api::Declaration& argOut)
{
    mConnection.call(vasum::api::host::METHOD_DECLARE_LINK, argIn, argOut);
}

void HostDbusConnection::callGetDeclarations(const vasum::api::ZoneId& argIn, vasum::api::Declarations& argOut)
{
    mConnection.call(vasum::api::host::METHOD_GET_DECLARATIONS, argIn, argOut);
}

void HostDbusConnection::callRemoveDeclaration(const vasum::api::RemoveDeclarationIn& argIn)
{
    mConnection.call(vasum::api::host::METHOD_REMOVE_DECLARATION, argIn);
}

void HostDbusConnection::callCreateZone(const vasum::api::CreateZoneIn& argIn)
{
    mConnection.call(vasum::api::host::METHOD_CREATE_ZONE, argIn);
}

void HostDbusConnection::callDestroyZone(const vasum::api::ZoneId& argIn)
{
    mConnection.call(vasum::api::host::METHOD_DESTROY_ZONE, argIn);
}

void HostDbusConnection::callShutdownZone(const vasum::api::ZoneId& argIn)
{
    mConnection.call(vasum::api::host::METHOD_SHUTDOWN_ZONE, argIn);
}

void HostDbusConnection::callStartZone(const vasum::api::ZoneId& argIn)
{
    mConnection.call(vasum::api::host::METHOD_START_ZONE, argIn);
}

void HostDbusConnection::callLockZone(const vasum::api::ZoneId& argIn)
{
    mConnection.call(vasum::api::host::METHOD_LOCK_ZONE, argIn);
}

void HostDbusConnection::callUnlockZone(const vasum::api::ZoneId& argIn)
{
    mConnection.call(vasum::api::host::METHOD_UNLOCK_ZONE, argIn);
}

void HostDbusConnection::callGrantDevice(const vasum::api::GrantDeviceIn& argIn)
{
    mConnection.call(vasum::api::host::METHOD_GRANT_DEVICE, argIn);
}

void HostDbusConnection::callRevokeDevice(const vasum::api::RevokeDeviceIn& argIn)
{
    mConnection.call(vasum::api::host::METHOD_REVOKE_DEVICE, argIn);
}

void HostDbusConnection::callGetZoneDbuses(vasum::api::Dbuses& argOut)
{
    mConnection.call(vasum::api::host::METHOD_GET_ZONE_DBUSES, argOut);
}

HostDbusConnection::SubscriptionId
HostDbusConnection::subscribeZoneDbusState(const ZoneDbusStateCallback& callback)
{
    return mConnection.signalSubscribe<vasum::api::DbusState>(
        vasum::api::host::SIGNAL_ZONE_DBUS_STATE, callback);
}

void HostDbusConnection::unsubscribe(const SubscriptionId& id)
{
    mConnection.signalUnsubscribe(id);
}

} // namespace client
} // namespace vasum
