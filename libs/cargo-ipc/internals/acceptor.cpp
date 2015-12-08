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
 * @brief   Class for accepting new connections
 */

#include "config.hpp"

#include "cargo-ipc/internals/acceptor.hpp"
#include "logger/logger.hpp"

#include <functional>

namespace cargo {
namespace ipc {
namespace internals {

Acceptor::Acceptor(epoll::EventPoll& eventPoll,
                   const std::string& socketPath,
                   const NewConnectionCallback& newConnectionCallback)
    : mEventPoll(eventPoll),
      mNewConnectionCallback(newConnectionCallback),
      mSocket(Socket::createUNIX(socketPath))
{
    LOGT("Creating Acceptor for socket " << socketPath);
    mEventPoll.addFD(mSocket.getFD(), EPOLLIN, std::bind(&Acceptor::handleConnection, this));
}

Acceptor::~Acceptor()
{
    LOGT("Destroyed Acceptor");
    mEventPoll.removeFD(mSocket.getFD());
}

void Acceptor::handleConnection()
{
    std::shared_ptr<Socket> tmpSocket = mSocket.accept();
    mNewConnectionCallback(tmpSocket);
}

} // namespace internals
} // namespace ipc
} // namespace cargo
