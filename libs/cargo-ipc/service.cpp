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
 * @brief   Implementation of the cargo IPC handling class
 */

#include "config.hpp"

#include "cargo-ipc/service.hpp"
#include "cargo-ipc/exception.hpp"
#include "logger/logger.hpp"

using namespace std::placeholders;
using namespace cargo::ipc::internals;

namespace cargo {
namespace ipc {

Service::Service(epoll::EventPoll& eventPoll,
                 const std::string& socketPath,
                 const PeerCallback& addPeerCallback,
                 const PeerCallback& removePeerCallback)
    : mEventPoll(eventPoll),
      mProcessor(eventPoll, "[SERVICE] "),
      mAcceptor(eventPoll, socketPath, std::bind(&Processor::addPeer, &mProcessor, _1))

{
    LOGS("Service Constructor");
    setNewPeerCallback(addPeerCallback);
    setRemovedPeerCallback(removePeerCallback);
}

Service::~Service()
{
    LOGS("Service Destructor");
    try {
        stop();
    } catch (std::exception& e) {
        LOGE("Error in Service's destructor: " << e.what());
    }
}

void Service::start()
{
    if (mProcessor.isStarted()) {
        return;
    }
    LOGS("Service start");

    mProcessor.start();
}

bool Service::isStarted()
{
    return mProcessor.isStarted();
}

void Service::stop(bool wait)
{
    if (!mProcessor.isStarted()) {
        return;
    }
    LOGS("Service stop");
    mProcessor.stop(wait);
}

void Service::handle(const FileDescriptor fd, const epoll::Events pollEvents)
{
    LOGS("Service handle");

    if (!isStarted()) {
        LOGW("Service stopped, but got event: " << pollEvents << " on fd: " << fd);
        return;
    }

    if ((pollEvents & EPOLLHUP) || (pollEvents & EPOLLRDHUP)) {
        //IN, HUP, RDHUP are set when client is disconnecting but there is 0 bytes to read, so
        //assume that if IN and HUP or RDHUP are set then input data is garbage.
        //Assumption is harmless because handleInput processes all message data.
        mProcessor.handleLostConnection(fd);
        return;
    }

    if (pollEvents & EPOLLIN) {
        //Process all message data
        mProcessor.handleInput(fd);
    }
}

void Service::setNewPeerCallback(const PeerCallback& newPeerCallback)
{
    LOGS("Service setNewPeerCallback");
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

void Service::setRemovedPeerCallback(const PeerCallback& removedPeerCallback)
{
    LOGS("Service setRemovedPeerCallback");
    auto callback = [removedPeerCallback, this](PeerID peerID, FileDescriptor fd) {
        mEventPoll.removeFD(fd);
        if (removedPeerCallback) {
            removedPeerCallback(peerID, fd);
        }
    };
    mProcessor.setRemovedPeerCallback(callback);
}

void Service::removeMethod(const MethodID methodID)
{
    LOGS("Service removeMethod methodID: " << methodID);
    mProcessor.removeMethod(methodID);
}

bool Service::isHandled(const MethodID methodID)
{
    return mProcessor.isHandled(methodID);
}

} // namespace ipc
} // namespace cargo
