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

namespace vasum {
namespace ipc {

Client::Client(const std::string& socketPath)
    : mSocketPath(socketPath)
{
    LOGS("Client Constructor");
}

Client::~Client()
{
    LOGS("Client Destructor");
    try {
        stop();
    } catch (IPCException& e) {
        LOGE("Error in Client's destructor: " << e.what());
    }
}

void Client::connect()
{
    // Initialize the connection with the server
    LOGD("Connecting to " + mSocketPath);
    auto socketPtr = std::make_shared<Socket>(Socket::connectSocket(mSocketPath));
    mServiceFD = mProcessor.addPeer(socketPtr);
}

void Client::start()
{
    LOGS("Client start");
    connect();
    mProcessor.start();
}

bool Client::isStarted()
{
    return mProcessor.isStarted();
}

void Client::stop()
{
    LOGS("Client Destructor");
    mProcessor.stop();
}

std::vector<FileDescriptor> Client::getFDs()
{
    std::vector<FileDescriptor> fds;
    fds.push_back(mProcessor.getEventFD());
    fds.push_back(mServiceFD);

    return fds;
}

void Client::handle(const FileDescriptor fd, const short pollEvent)
{
    if (fd == mProcessor.getEventFD() && (pollEvent & POLLIN)) {
        mProcessor.handleEvent();
        return;

    } else if (pollEvent & POLLIN) {
        mProcessor.handleInput(fd);
        return;

    } else if (pollEvent & POLLHUP) {
        mProcessor.handleLostConnection(fd);
        return;
    }
}

void Client::setNewPeerCallback(const PeerCallback& newPeerCallback)
{
    LOGS("Client setNewPeerCallback");
    mProcessor.setNewPeerCallback(newPeerCallback);
}

void Client::setRemovedPeerCallback(const PeerCallback& removedPeerCallback)
{
    LOGS("Client setRemovedPeerCallback");
    mProcessor.setRemovedPeerCallback(removedPeerCallback);
}

void Client::removeMethod(const MethodID methodID)
{
    LOGS("Client removeMethod methodID: " << methodID);
    mProcessor.removeMethod(methodID);
}

} // namespace ipc
} // namespace vasum
