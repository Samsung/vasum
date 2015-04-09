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
 * @brief   IPCConnection class
 */

#include <config.hpp>
#include "ipc-connection.hpp"

namespace {

const std::string SOCKET_PATH = HOST_IPC_SOCKET;

} // namespace

vasum::client::IPCConnection::IPCConnection()
{
}

vasum::client::IPCConnection::~IPCConnection()
{
}

void vasum::client::IPCConnection::createSystem()
{
    mClient.reset(new ipc::Client(mDispatcher.getPoll(), SOCKET_PATH));
    mClient->start();
}
