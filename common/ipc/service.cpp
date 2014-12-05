// /*
// *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
// *
// *  Contact: Jan Olszak <j.olszak@samsung.com>
// *
// *  Licensed under the Apache License, Version 2.0 (the "License");
// *  you may not use this file except in compliance with the License.
// *  You may obtain a copy of the License at
// *
// *      http://www.apache.org/licenses/LICENSE-2.0
// *
// *  Unless required by applicable law or agreed to in writing, software
// *  distributed under the License is distributed on an "AS IS" BASIS,
// *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// *  See the License for the specific language governing permissions and
// *  limitations under the License
// */

// /**
//  * @file
//  * @author  Jan Olszak (j.olszak@samsung.com)
//  * @brief   Implementation of the IPC handling class
//  */

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
    : mProcessor(addPeerCallback, removePeerCallback),
      mAcceptor(socketPath, std::bind(&Processor::addPeer, &mProcessor, _1))

{
    LOGD("Creating server");
}

Service::~Service()
{
    LOGD("Destroying server...");
    try {
        stop();
    } catch (IPCException& e) {
        LOGE("Error in Service's destructor: " << e.what());
    }
    LOGD("Destroyed");
}

void Service::start()
{
    LOGD("Starting server");
    mProcessor.start();

    // There can be an incoming connection from mAcceptor before mProcessor is listening,
    // but it's OK. It will handle the connection when ready. So no need to wait for mProcessor.
    mAcceptor.start();

    LOGD("Started server");
}

bool Service::isStarted()
{
    return mProcessor.isStarted();
}

void Service::stop()
{
    LOGD("Stopping server..");
    mAcceptor.stop();
    mProcessor.stop();
    LOGD("Stopped");
}

void Service::setNewPeerCallback(const PeerCallback& newPeerCallback)
{
    mProcessor.setNewPeerCallback(newPeerCallback);
}

void Service::setRemovedPeerCallback(const PeerCallback& removedPeerCallback)
{
    mProcessor.setRemovedPeerCallback(removedPeerCallback);
}

void Service::removeMethod(const MethodID methodID)
{
    LOGD("Removing method " << methodID);
    mProcessor.removeMethod(methodID);
    LOGD("Removed " << methodID);
}


} // namespace ipc
} // namespace vasum
