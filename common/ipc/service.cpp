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
 * @brief   Implementation of the IPC handling class
 */

#include "config.hpp"

#include "ipc/service.hpp"
#include "ipc/exception.hpp"
#include "logger/logger.hpp"

using namespace std::placeholders;

namespace vasum {
namespace ipc {

Service::Service(const std::string& socketPath,
                 const PeerCallback& addPeerCallback,
                 const PeerCallback& removePeerCallback)
    : mProcessor("[SERVICE] "),
      mAcceptor(socketPath, std::bind(&Processor::addPeer, &mProcessor, _1))

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

void Service::start(const bool usesExternalPolling)
{
    LOGS("Service start");
    if (usesExternalPolling) {
        startPoll();
    }
    mProcessor.start(usesExternalPolling);

    // There can be an incoming connection from mAcceptor before mProcessor is listening,
    // but it's OK. It will handle the connection when ready. So no need to wait for mProcessor.
    if (!usesExternalPolling) {
        mAcceptor.start();
    }
}

bool Service::isStarted()
{
    return mProcessor.isStarted();
}

void Service::stop()
{
    LOGS("Service stop");
    mAcceptor.stop();
    mProcessor.stop();

    if (mIPCGSourcePtr) {
        stopPoll();
    }
}

void Service::startPoll()
{
    LOGS("Service startPoll");

    mIPCGSourcePtr = IPCGSource::create(std::bind(&Service::handle, this, _1, _2));
    mIPCGSourcePtr->addFD(mAcceptor.getEventFD());
    mIPCGSourcePtr->addFD(mAcceptor.getConnectionFD());
    mIPCGSourcePtr->addFD(mProcessor.getEventFD());
    mIPCGSourcePtr->attach();
}

void Service::stopPoll()
{
    LOGS("Service stopPoll");

    mIPCGSourcePtr->removeFD(mAcceptor.getEventFD());
    mIPCGSourcePtr->removeFD(mAcceptor.getConnectionFD());
    mIPCGSourcePtr->removeFD(mProcessor.getEventFD());
    mIPCGSourcePtr->detach();
    mIPCGSourcePtr.reset();
}

void Service::handle(const FileDescriptor fd, const short pollEvent)
{
    LOGS("Service handle");

    if (!isStarted()) {
        LOGW("Service stopped");
        return;
    }

    if (fd == mProcessor.getEventFD() && (pollEvent & POLLIN)) {
        mProcessor.handleEvent();
        return;

    } else if (fd == mAcceptor.getConnectionFD() && (pollEvent & POLLIN)) {
        mAcceptor.handleConnection();
        return;

    } else if (fd == mAcceptor.getEventFD() && (pollEvent & POLLIN)) {
        mAcceptor.handleEvent();
        return;

    } else if (pollEvent & POLLIN) {
        mProcessor.handleInput(fd);
        return;

    } else if (pollEvent & POLLHUP) {
        mProcessor.handleLostConnection(fd);
        return;
    }
}

void Service::setNewPeerCallback(const PeerCallback& newPeerCallback)
{
    LOGS("Service setNewPeerCallback");
    auto callback = [newPeerCallback, this](PeerID peerID, FileDescriptor fd) {
        if (mIPCGSourcePtr) {
            mIPCGSourcePtr->addFD(fd);
        }
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
        if (mIPCGSourcePtr) {
            mIPCGSourcePtr->removeFD(fd);
        }
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


} // namespace ipc
} // namespace vasum
