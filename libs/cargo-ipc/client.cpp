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

#include "cargo-ipc/client.hpp"
#include "cargo-ipc/internals/socket.hpp"
#include "cargo-ipc/exception.hpp"

namespace cargo {
namespace ipc {

using namespace internals;

Client::Client(epoll::EventPoll& eventPoll, const std::string& socketPath)
    : mEventPoll(eventPoll),
      mServiceID(),
      mProcessor(eventPoll, "[CLIENT]  "),
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
    } catch (std::exception& e) {
        LOGE("Error in Client's destructor: " << e.what());
    }
}

void Client::start()
{
    if (mProcessor.isStarted()) {
        return;
    }
    LOGS("Client start");
    LOGD("Connecting to " + mSocketPath);
    auto socketPtr = std::make_shared<Socket>(Socket::connectUNIX(mSocketPath));

    mProcessor.start();

    mServiceID = mProcessor.addPeer(socketPtr);
}

bool Client::isStarted()
{
    return mProcessor.isStarted();
}

void Client::stop(bool wait)
{
    if (!mProcessor.isStarted()) {
        return;
    }
    LOGS("Client stop");
    mProcessor.stop(wait);
}

void Client::handle(const FileDescriptor fd, const epoll::Events pollEvents)
{
    LOGS("Client handle");

    if (!isStarted()) {
        LOGW("Client stopped");
        return;
    }

    if (pollEvents & EPOLLIN) {
        mProcessor.handleInput(fd);
        return; // because handleInput will handle RDHUP
    }

    if ((pollEvents & EPOLLHUP) || (pollEvents & EPOLLRDHUP)) {
        mProcessor.handleLostConnection(fd);
    }
}

void Client::setNewPeerCallback(const PeerCallback& newPeerCallback)
{
    LOGS("Client setNewPeerCallback");
    auto callback = [newPeerCallback, this](PeerID peerID, FileDescriptor fd) {
        auto handleFd = [&](FileDescriptor fd, epoll::Events events) {
            handle(fd, events);
        };
        mEventPoll.addFD(fd, EPOLLIN | EPOLLHUP | EPOLLRDHUP, handleFd);
        if (newPeerCallback) {
            newPeerCallback(peerID, fd);
        }
    };
    mProcessor.setNewPeerCallback(callback);
}

void Client::setRemovedPeerCallback(const PeerCallback& removedPeerCallback)
{
    LOGS("Client setRemovedPeerCallback");
    auto callback = [removedPeerCallback, this](PeerID peerID, FileDescriptor fd) {
        mEventPoll.removeFD(fd);
        if (removedPeerCallback) {
            removedPeerCallback(peerID, fd);
        }
    };
    mProcessor.setRemovedPeerCallback(callback);
}

void Client::removeMethod(const MethodID methodID)
{
    LOGS("Client removeMethod methodID: " << methodID);
    mProcessor.removeMethod(methodID);
}

bool Client::isHandled(const MethodID methodID)
{
    return mProcessor.isHandled(methodID);
}

} // namespace ipc
} // namespace cargo
