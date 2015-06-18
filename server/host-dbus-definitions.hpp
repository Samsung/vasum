/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Piotr Bartosiewicz <p.bartosiewi@partner.samsung.com>
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
 * @author  Piotr Bartosiewicz (p.bartosiewi@partner.samsung.com)
 * @brief   Host dbus api definitions
 */

#ifndef SERVER_HOST_DBUS_DEFINITIONS_HPP
#define SERVER_HOST_DBUS_DEFINITIONS_HPP

#include <string>

namespace vasum {
namespace api {
namespace dbus {

const std::string BUS_NAME                        = "org.tizen.vasum.host";
const std::string OBJECT_PATH                     = "/org/tizen/vasum/host";
const std::string INTERFACE                       = "org.tizen.vasum.host.manager";


const std::string METHOD_GET_ZONE_ID_LIST         = "GetZoneIds";
const std::string METHOD_GET_ACTIVE_ZONE_ID       = "GetActiveZoneId";
const std::string METHOD_GET_ZONE_INFO            = "GetZoneInfo";
const std::string METHOD_SET_NETDEV_ATTRS         = "SetNetdevAttrs";
const std::string METHOD_GET_NETDEV_ATTRS         = "GetNetdevAttrs";
const std::string METHOD_GET_NETDEV_LIST          = "GetNetdevList";
const std::string METHOD_CREATE_NETDEV_VETH       = "CreateNetdevVeth";
const std::string METHOD_CREATE_NETDEV_MACVLAN    = "CreateNetdevMacvlan";
const std::string METHOD_CREATE_NETDEV_PHYS       = "CreateNetdevPhys";
const std::string METHOD_DESTROY_NETDEV           = "DestroyNetdev";
const std::string METHOD_DELETE_NETDEV_IP_ADDRESS = "DeleteNetdevIpAddress";
const std::string METHOD_DECLARE_FILE             = "DeclareFile";
const std::string METHOD_DECLARE_MOUNT            = "DeclareMount";
const std::string METHOD_DECLARE_LINK             = "DeclareLink";
const std::string METHOD_GET_DECLARATIONS         = "GetDeclarations";
const std::string METHOD_REMOVE_DECLARATION       = "RemoveDeclaration";
const std::string METHOD_SET_ACTIVE_ZONE          = "SetActiveZone";
const std::string METHOD_CREATE_ZONE              = "CreateZone";
const std::string METHOD_DESTROY_ZONE             = "DestroyZone";
const std::string METHOD_SHUTDOWN_ZONE            = "ShutdownZone";
const std::string METHOD_START_ZONE               = "StartZone";
const std::string METHOD_LOCK_ZONE                = "LockZone";
const std::string METHOD_UNLOCK_ZONE              = "UnlockZone";
const std::string METHOD_GRANT_DEVICE             = "GrantDevice";
const std::string METHOD_REVOKE_DEVICE            = "RevokeDevice";
const std::string METHOD_PROXY_CALL               = "ProxyCall";
const std::string METHOD_CREATE_FILE              = "CreateFile";
const std::string METHOD_LOCK_QUEUE               = "LockQueue";
const std::string METHOD_UNLOCK_QUEUE             = "UnlockQueue";
const std::string METHOD_SWITCH_TO_DEFAULT        = "SwitchToDefault";

const std::string METHOD_NOTIFY_ACTIVE_ZONE         = "NotifyActiveZone";
const std::string METHOD_FILE_MOVE_REQUEST          = "FileMoveRequest";
const std::string SIGNAL_NOTIFICATION               = "Notification";

const std::string FILE_MOVE_DESTINATION_NOT_FOUND   = "FILE_MOVE_DESTINATION_NOT_FOUND";
const std::string FILE_MOVE_WRONG_DESTINATION       = "FILE_MOVE_WRONG_DESTINATION";
const std::string FILE_MOVE_NO_PERMISSIONS_SEND     = "FILE_MOVE_NO_PERMISSIONS_SEND";
const std::string FILE_MOVE_NO_PERMISSIONS_RECEIVE  = "FILE_MOVE_NO_PERMISSIONS_RECEIVE";
const std::string FILE_MOVE_FAILED                  = "FILE_MOVE_FAILED";
const std::string FILE_MOVE_SUCCEEDED               = "FILE_MOVE_SUCCEEDED";


const std::string DEFINITION =
    "<node>"
    "  <interface name='" + INTERFACE + "'>"
    "    <method name='" + METHOD_LOCK_QUEUE + "'>"
    "    </method>"
    "    <method name='" + METHOD_UNLOCK_QUEUE + "'>"
    "    </method>"
    "    <method name='" + METHOD_GET_ZONE_ID_LIST + "'>"
    "      <arg type='as' name='result' direction='out'/>"
    "    </method>"
    "    <method name='" + METHOD_GET_ACTIVE_ZONE_ID + "'>"
    "      <arg type='s' name='result' direction='out'/>"
    "    </method>"
    "    <method name='" + METHOD_GET_ZONE_INFO + "'>"
    "      <arg type='s' name='id' direction='in'/>"
    "      <arg type='s' name='id' direction='out'/>"
    "      <arg type='i' name='vt' direction='out'/>"
    "      <arg type='s' name='state' direction='out'/>"
    "      <arg type='s' name='rootPath' direction='out'/>"
    "    </method>"
    "    <method name='" + METHOD_SET_NETDEV_ATTRS + "'>"
    "      <arg type='s' name='zone' direction='in'/>"
    "      <arg type='s' name='netdev' direction='in'/>"
    "      <arg type='a(ss)' name='attributes' direction='in'/>"
    "    </method>"
    "    <method name='" + METHOD_GET_NETDEV_ATTRS + "'>"
    "      <arg type='s' name='zone' direction='in'/>"
    "      <arg type='s' name='netdev' direction='in'/>"
    "      <arg type='a(ss)' name='attributes' direction='out'/>"
    "    </method>"
    "    <method name='" + METHOD_GET_NETDEV_LIST + "'>"
    "      <arg type='s' name='zone' direction='in'/>"
    "      <arg type='as' name='list' direction='out'/>"
    "    </method>"
    "    <method name='" + METHOD_CREATE_NETDEV_VETH + "'>"
    "      <arg type='s' name='id' direction='in'/>"
    "      <arg type='s' name='zoneDev' direction='in'/>"
    "      <arg type='s' name='hostDev' direction='in'/>"
    "    </method>"
    "    <method name='" + METHOD_CREATE_NETDEV_MACVLAN + "'>"
    "      <arg type='s' name='id' direction='in'/>"
    "      <arg type='s' name='zoneDev' direction='in'/>"
    "      <arg type='s' name='hostDev' direction='in'/>"
    "      <arg type='u' name='mode' direction='in'/>"
    "    </method>"
    "    <method name='" + METHOD_CREATE_NETDEV_PHYS + "'>"
    "      <arg type='s' name='id' direction='in'/>"
    "      <arg type='s' name='devId' direction='in'/>"
    "    </method>"
    "    <method name='" + METHOD_DESTROY_NETDEV + "'>"
    "      <arg type='s' name='id' direction='in'/>"
    "      <arg type='s' name='devId' direction='in'/>"
    "    </method>"
    "    <method name='" + METHOD_DELETE_NETDEV_IP_ADDRESS + "'>"
    "      <arg type='s' name='id' direction='in'/>"
    "      <arg type='s' name='devId' direction='in'/>"
    "      <arg type='s' name='ip' direction='in'/>"
    "    </method>"
    "    <method name='" + METHOD_DECLARE_FILE + "'>"
    "      <arg type='s' name='zone' direction='in'/>"
    "      <arg type='i' name='type' direction='in'/>"
    "      <arg type='s' name='path' direction='in'/>"
    "      <arg type='i' name='flags' direction='in'/>"
    "      <arg type='i' name='mode' direction='in'/>"
    "      <arg type='s' name='id' direction='out'/>"
    "    </method>"
    "    <method name='" + METHOD_DECLARE_MOUNT + "'>"
    "      <arg type='s' name='source' direction='in'/>"
    "      <arg type='s' name='zone' direction='in'/>"
    "      <arg type='s' name='target' direction='in'/>"
    "      <arg type='s' name='type' direction='in'/>"
    "      <arg type='t' name='flags' direction='in'/>"
    "      <arg type='s' name='data' direction='in'/>"
    "      <arg type='s' name='id' direction='out'/>"
    "    </method>"
    "    <method name='" + METHOD_DECLARE_LINK + "'>"
    "      <arg type='s' name='source' direction='in'/>"
    "      <arg type='s' name='zone' direction='in'/>"
    "      <arg type='s' name='target' direction='in'/>"
    "      <arg type='s' name='id' direction='out'/>"
    "    </method>"
    "    <method name='" + METHOD_GET_DECLARATIONS + "'>"
    "      <arg type='s' name='zone' direction='in'/>"
    "      <arg type='as' name='list' direction='out'/>"
    "    </method>"
    "    <method name='" + METHOD_REMOVE_DECLARATION + "'>"
    "      <arg type='s' name='zone' direction='in'/>"
    "      <arg type='s' name='declaration' direction='in'/>"
    "    </method>"
    "    <method name='" + METHOD_SET_ACTIVE_ZONE + "'>"
    "      <arg type='s' name='id' direction='in'/>"
    "    </method>"
    "    <method name='" + METHOD_CREATE_ZONE + "'>"
    "      <arg type='s' name='id' direction='in'/>"
    "      <arg type='s' name='templateName' direction='in'/>"
    "    </method>"
    "    <method name='" + METHOD_CREATE_FILE + "'>"
    "      <arg type='s' name='id' direction='in'/>"
    "      <arg type='s' name='path' direction='in'/>"
    "      <arg type='i' name='flags' direction='in'/>"
    "      <arg type='i' name='mode' direction='in'/>"
    "      <arg type='h' name='fileDescriptor' direction='out'/>"
    "    </method>"
    "    <method name='" + METHOD_DESTROY_ZONE + "'>"
    "      <arg type='s' name='id' direction='in'/>"
    "    </method>"
    "    <method name='" + METHOD_SHUTDOWN_ZONE + "'>"
    "      <arg type='s' name='id' direction='in'/>"
    "    </method>"
    "    <method name='" + METHOD_START_ZONE + "'>"
    "      <arg type='s' name='id' direction='in'/>"
    "    </method>"
    "    <method name='" + METHOD_LOCK_ZONE + "'>"
    "      <arg type='s' name='id' direction='in'/>"
    "    </method>"
    "    <method name='" + METHOD_UNLOCK_ZONE + "'>"
    "      <arg type='s' name='id' direction='in'/>"
    "    </method>"
    "    <method name='" + METHOD_GRANT_DEVICE + "'>"
    "      <arg type='s' name='id' direction='in'/>"
    "      <arg type='s' name='device' direction='in'/>"
    "      <arg type='u' name='flags' direction='in'/>"
    "    </method>"
    "    <method name='" + METHOD_REVOKE_DEVICE + "'>"
    "      <arg type='s' name='id' direction='in'/>"
    "      <arg type='s' name='device' direction='in'/>"
    "    </method>"
    "    <method name='" + METHOD_NOTIFY_ACTIVE_ZONE + "'>"
    "      <arg type='s' name='application' direction='in'/>"
    "      <arg type='s' name='message' direction='in'/>"
    "    </method>"
    "    <method name='" + METHOD_FILE_MOVE_REQUEST + "'>"
    "      <arg type='s' name='destination' direction='in'/>"
    "      <arg type='s' name='path' direction='in'/>"
    "      <arg type='s' name='result' direction='out'/>"
    "    </method>"
    "    <method name='" + METHOD_PROXY_CALL + "'>"
    "      <arg type='s' name='target' direction='in'/>"
    "      <arg type='s' name='busName' direction='in'/>"
    "      <arg type='s' name='objectPath' direction='in'/>"
    "      <arg type='s' name='interface' direction='in'/>"
    "      <arg type='s' name='method' direction='in'/>"
    "      <arg type='v' name='parameters' direction='in'/>"
    "      <arg type='v' name='result' direction='out'/>"
    "    </method>"
    "    <method name='" + METHOD_SWITCH_TO_DEFAULT + "'>"
    "    </method>"
    "    <signal name='" + SIGNAL_NOTIFICATION + "'>"
    "      <arg type='s' name='zone'/>"
    "      <arg type='s' name='application'/>"
    "      <arg type='s' name='message'/>"
    "    </signal>"
    "  </interface>"
    "</node>";

} // namespace dbus
} // namespace api
} // namespace vasum


#endif // SERVER_HOST_DBUS_DEFINITIONS_HPP
