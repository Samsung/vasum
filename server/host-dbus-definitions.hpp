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

#include "common-dbus-definitions.hpp"


namespace vasum {
namespace api {
namespace host {

const std::string BUS_NAME                       = "org.tizen.vasum.host";
const std::string OBJECT_PATH                    = "/org/tizen/vasum/host";
const std::string INTERFACE                      = "org.tizen.vasum.host.manager";

const std::string ERROR_ZONE_STOPPED        = "org.tizen.vasum.host.Error.ZonesStopped";

const std::string METHOD_GET_ZONE_DBUSES    = "GetZoneDbuses";
const std::string METHOD_GET_ZONE_ID_LIST   = "GetZoneIds";
const std::string METHOD_GET_ACTIVE_ZONE_ID = "GetActiveZoneId";
const std::string METHOD_GET_ZONE_INFO      = "GetZoneInfo";
const std::string METHOD_DECLARE_FILE            = "DeclareFile";
const std::string METHOD_DECLARE_MOUNT           = "DeclareMount";
const std::string METHOD_DECLARE_LINK            = "DeclareLink";
const std::string METHOD_SET_ACTIVE_ZONE    = "SetActiveZone";
const std::string METHOD_CREATE_ZONE        = "CreateZone";
const std::string METHOD_DESTROY_ZONE       = "DestroyZone";
const std::string METHOD_LOCK_ZONE          = "LockZone";
const std::string METHOD_UNLOCK_ZONE        = "UnlockZone";

const std::string SIGNAL_ZONE_DBUS_STATE    = "ZoneDbusState";


const std::string DEFINITION =
    "<node>"
    "  <interface name='" + INTERFACE + "'>"
    "    <method name='" + METHOD_PROXY_CALL + "'>"
    "      <arg type='s' name='target' direction='in'/>"
    "      <arg type='s' name='busName' direction='in'/>"
    "      <arg type='s' name='objectPath' direction='in'/>"
    "      <arg type='s' name='interface' direction='in'/>"
    "      <arg type='s' name='method' direction='in'/>"
    "      <arg type='v' name='parameters' direction='in'/>"
    "      <arg type='v' name='result' direction='out'/>"
    "    </method>"
    "    <method name='" + METHOD_GET_ZONE_DBUSES + "'>"
    "      <arg type='a{ss}' name='dbuses' direction='out'/>"
    "    </method>"
    "    <method name='" + METHOD_GET_ZONE_ID_LIST + "'>"
    "      <arg type='as' name='result' direction='out'/>"
    "    </method>"
    "    <method name='" + METHOD_GET_ACTIVE_ZONE_ID + "'>"
    "      <arg type='s' name='result' direction='out'/>"
    "    </method>"
    "    <method name='" + METHOD_GET_ZONE_INFO + "'>"
    "      <arg type='s' name='id' direction='in'/>"
    "      <arg type='(siss)' name='result' direction='out'/>"
    "    </method>"
    "    <method name='" + METHOD_DECLARE_FILE + "'>"
    "      <arg type='s' name='zone' direction='in'/>"
    "      <arg type='i' name='type' direction='in'/>"
    "      <arg type='s' name='path' direction='in'/>"
    "      <arg type='i' name='flags' direction='in'/>"
    "      <arg type='i' name='mode' direction='in'/>"
    "    </method>"
    "    <method name='" + METHOD_DECLARE_MOUNT + "'>"
    "      <arg type='s' name='source' direction='in'/>"
    "      <arg type='s' name='zone' direction='in'/>"
    "      <arg type='s' name='target' direction='in'/>"
    "      <arg type='s' name='type' direction='in'/>"
    "      <arg type='t' name='flags' direction='in'/>"
    "      <arg type='s' name='data' direction='in'/>"
    "    </method>"
    "    <method name='" + METHOD_DECLARE_LINK + "'>"
    "      <arg type='s' name='source' direction='in'/>"
    "      <arg type='s' name='zone' direction='in'/>"
    "      <arg type='s' name='target' direction='in'/>"
    "    </method>"
    "    <method name='" + METHOD_SET_ACTIVE_ZONE + "'>"
    "      <arg type='s' name='id' direction='in'/>"
    "    </method>"
    "    <method name='" + METHOD_CREATE_ZONE + "'>"
    "      <arg type='s' name='id' direction='in'/>"
    "    </method>"
    "    <method name='" + METHOD_DESTROY_ZONE + "'>"
    "      <arg type='s' name='id' direction='in'/>"
    "    </method>"
    "    <method name='" + METHOD_LOCK_ZONE + "'>"
    "      <arg type='s' name='id' direction='in'/>"
    "    </method>"
    "    <method name='" + METHOD_UNLOCK_ZONE + "'>"
    "      <arg type='s' name='id' direction='in'/>"
    "    </method>"
    "    <signal name='" + SIGNAL_ZONE_DBUS_STATE + "'>"
    "      <arg type='s' name='zone'/>"
    "      <arg type='s' name='dbusAddress'/>"
    "    </signal>"
    "  </interface>"
    "</node>";

} // namespace host
} // namespace api
} // namespace vasum


#endif // SERVER_HOST_DBUS_DEFINITIONS_HPP
