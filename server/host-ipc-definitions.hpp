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
 * @brief   Host ipc api definitions
 */

#ifndef SERVER_HOST_IPC_DEFINITIONS_HPP
#define SERVER_HOST_IPC_DEFINITIONS_HPP

#include "ipc/types.hpp"

namespace vasum {
namespace api {

const vasum::ipc::MethodID METHOD_GET_ZONE_ID_LIST         = 2;
const vasum::ipc::MethodID METHOD_GET_ACTIVE_ZONE_ID       = 3;
const vasum::ipc::MethodID METHOD_GET_ZONE_INFO            = 4;
const vasum::ipc::MethodID METHOD_SET_NETDEV_ATTRS         = 5;
const vasum::ipc::MethodID METHOD_GET_NETDEV_ATTRS         = 6;
const vasum::ipc::MethodID METHOD_GET_NETDEV_LIST          = 7;
const vasum::ipc::MethodID METHOD_CREATE_NETDEV_VETH       = 8;
const vasum::ipc::MethodID METHOD_CREATE_NETDEV_MACVLAN    = 9;
const vasum::ipc::MethodID METHOD_CREATE_NETDEV_PHYS       = 10;
const vasum::ipc::MethodID METHOD_DELETE_NETDEV_IP_ADDRESS = 11;
const vasum::ipc::MethodID METHOD_DESTROY_NETDEV           = 12;
const vasum::ipc::MethodID METHOD_DECLARE_FILE             = 13;
const vasum::ipc::MethodID METHOD_DECLARE_MOUNT            = 14;
const vasum::ipc::MethodID METHOD_DECLARE_LINK             = 15;
const vasum::ipc::MethodID METHOD_GET_DECLARATIONS         = 16;
const vasum::ipc::MethodID METHOD_REMOVE_DECLARATION       = 17;
const vasum::ipc::MethodID METHOD_SET_ACTIVE_ZONE          = 18;
const vasum::ipc::MethodID METHOD_CREATE_ZONE              = 19;
const vasum::ipc::MethodID METHOD_DESTROY_ZONE             = 20;
const vasum::ipc::MethodID METHOD_SHUTDOWN_ZONE            = 21;
const vasum::ipc::MethodID METHOD_START_ZONE               = 22;
const vasum::ipc::MethodID METHOD_LOCK_ZONE                = 23;
const vasum::ipc::MethodID METHOD_UNLOCK_ZONE              = 24;
const vasum::ipc::MethodID METHOD_GRANT_DEVICE             = 25;
const vasum::ipc::MethodID METHOD_REVOKE_DEVICE            = 26;

const vasum::ipc::MethodID METHOD_NOTIFY_ACTIVE_ZONE         = 100;
const vasum::ipc::MethodID METHOD_FILE_MOVE_REQUEST          = 101;
const vasum::ipc::MethodID SIGNAL_NOTIFICATION               = 102;
const vasum::ipc::MethodID SIGNAL_SWITCH_TO_DEFAULT          = 103;

const std::string FILE_MOVE_DESTINATION_NOT_FOUND   = "FILE_MOVE_DESTINATION_NOT_FOUND";
const std::string FILE_MOVE_WRONG_DESTINATION       = "FILE_MOVE_WRONG_DESTINATION";
const std::string FILE_MOVE_NO_PERMISSIONS_SEND     = "FILE_MOVE_NO_PERMISSIONS_SEND";
const std::string FILE_MOVE_NO_PERMISSIONS_RECEIVE  = "FILE_MOVE_NO_PERMISSIONS_RECEIVE";
const std::string FILE_MOVE_FAILED                  = "FILE_MOVE_FAILED";
const std::string FILE_MOVE_SUCCEEDED               = "FILE_MOVE_SUCCEEDED";

} // namespace api
} // namespace vasum


#endif // SERVER_HOST_IPC_DEFINITIONS_HPP
