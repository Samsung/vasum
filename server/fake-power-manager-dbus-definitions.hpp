/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Lukasz Kostyra <l.kostyra@samsung.com>
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
 * @file    fake-power-manager-dbus-definitions.h
 * @author  Lukasz Kostyra (l.kostyra@samsung.com)
 * @brief   Declaration of fake dbus definitions from power-manager. Made only to test API in
 *          ZoneConnection.
 */

#ifndef FAKE_POWER_MANAGER_DBUS_DEFINITIONS_H
#define FAKE_POWER_MANAGER_DBUS_DEFINITIONS_H

/**
 * !!WARNING!!
 *
 * This header file is created only to test if API in ZoneConnection works correctly. It should
 * be removed when power-managers API will be created.
 */

namespace fake_power_manager_api
{

const std::string BUS_NAME                          = "com.tizen.fakepowermanager";
const std::string OBJECT_PATH                       = "/com/tizen/fakepowermanager";
const std::string INTERFACE                         = "com.tizen.fakepowermanager.manager";

const std::string SIGNAL_DISPLAY_OFF                = "DisplayOff";

const std::string DEFINITION =
    "<node>"
    "  <interface name='" + INTERFACE + "'>"
    "    <signal name='" + SIGNAL_DISPLAY_OFF + "'>"
    "    </signal>"
    "  </interface>"
    "</node>";


} // namespace fake_power_manager_api

#endif // FAKE_POWER_MANAGER_DBUS_DEFINITIONS_H
