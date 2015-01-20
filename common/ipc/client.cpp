/*
*  Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
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
    : mProcessor("[CLIENT]  "),
      mSocketPath(socketPath)
{
    LOGS("Client Constructor");
    setNewPeerCallback(nullptr);
    setRemovedPeerCallback(nullptr);
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

void Client::start(const bool usesExternalPolling)
{
    LOGS("Client start");
    // Initialize the connection with the server
    if (usesExternalPolling) {
        startPoll();
    }
    mProcessor.start(usesExternalPolling);

    LOGD("Connecting to " + mSocketPath);
    auto socketPtr = std::make_shared<Socket>(Socket::connectSocket(mSocketPath));
    mServiceFD = mProcessor.addPeer(socketPtr);
}

bool Client::isStarted()
{
    return mProcessor.isStarted();
}

void Client::stop()
{
    LOGS("Client stop");
    mProcessor.stop();

    if (mIPCGSourcePtr) {
        stopPoll();
    }
}

void Client::startPoll()
{
    LOGS("Client startPoll");
    using namespace std::placeholders;
    mIPCGSourcePtr = IPCGSource::create(std::bind(&Client::handle, this, _1, _2));
    mIPCGSourcePtr->addFD(mProcessor.getEventFD());
    mIPCGSourcePtr->attach();
}

void Client::stopPoll()
{
    LOGS("Client stopPoll");

    mIPCGSourcePtr->removeFD(mProcessor.getEventFD());
    mIPCGSourcePtr->detach();
    mIPCGSourcePtr.reset();
}

void Client::handle(const FileDescriptor fd, const short pollEvent)
{
    LOGS("Client handle");

    if (!isStarted()) {
        LOGW("Client stopped");
        return;
    }

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
    auto callback = [newPeerCallback, this](FileDescriptor fd) {
        if (mIPCGSourcePtr) {
            mIPCGSourcePtr->addFD(fd);
        }
        if (newPeerCallback) {
            newPeerCallback(fd);
        }
    };
    mProcessor.setNewPeerCallback(callback);
}

void Client::setRemovedPeerCallback(const PeerCallback& removedPeerCallback)
{
    LOGS("Client setRemovedPeerCallback");
    auto callback = [removedPeerCallback, this](FileDescriptor fd) {
        if (mIPCGSourcePtr) {
            mIPCGSourcePtr->removeFD(fd);
        }
        if (removedPeerCallback) {
            removedPeerCallback(fd);
        }
    };
    mProcessor.setRemovedPeerCallback(callback);
}

void Client::removeMethod(const MethodID methodID)
{
    LOGS("Client removeMethod methodID: " << methodID);
    mProcessor.removeMethod(methodID);
}

} // namespace ipc
} // namespace vasum
