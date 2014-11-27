/*
*  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
*
*  Contact: Jan Olszak <j.olszak@samsung.com>
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
 * @author  Jan Olszak (j.olszak@samsung.com)
 * @brief   Handling client connections
 */

#include "config.hpp"

#include "ipc/client.hpp"
#include "ipc/internals/socket.hpp"
#include "ipc/exception.hpp"

namespace security_containers {
namespace ipc {

Client::Client(const std::string& socketPath)
    : mSocketPath(socketPath)
{
    LOGD("Creating client");
}

Client::~Client()
{
    LOGD("Destroying client...");
    try {
        stop();
    } catch (IPCException& e) {
        LOGE("Error in Client's destructor: " << e.what());
    }
    LOGD("Destroyed client");
}

void Client::start()
{
    LOGD("Starting client...");

    // Initialize the connection with the server
    LOGD("Connecting to " + mSocketPath);
    auto socketPtr = std::make_shared<Socket>(Socket::connectSocket(mSocketPath));
    mServiceID = mProcessor.addPeer(socketPtr);

    // Start listening
    mProcessor.start();

    LOGD("Started client");
}

bool Client::isStarted()
{
    return mProcessor.isStarted();
}

void Client::stop()
{
    LOGD("Stopping client...");
    mProcessor.stop();
    LOGD("Stopped");
}

void Client::setNewPeerCallback(const PeerCallback& newPeerCallback)
{
    mProcessor.setNewPeerCallback(newPeerCallback);
}

void Client::setRemovedPeerCallback(const PeerCallback& removedPeerCallback)
{
    mProcessor.setRemovedPeerCallback(removedPeerCallback);
}

void Client::removeMethod(const MethodID methodID)
{
    LOGD("Removing method id: " << methodID);
    mProcessor.removeMethod(methodID);
}

} // namespace ipc
} // namespace security_containers
