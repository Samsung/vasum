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

#ifndef CARGO_IPC_INTERNALS_ACCEPTOR_HPP
#define CARGO_IPC_INTERNALS_ACCEPTOR_HPP

#include "cargo-ipc/internals/socket.hpp"
#include "cargo-ipc/epoll/event-poll.hpp"
#include "cargo-ipc/types.hpp"

#include <string>

namespace cargo {
namespace ipc {
namespace internals {

/**
 * Accepts new connections and passes the new socket to a callback.
 */
class Acceptor {
public:

    typedef std::function<void(std::shared_ptr<Socket>& socketPtr)> NewConnectionCallback;

    /**
     * Class for accepting new connections.
     *
     * @param eventPoll dispatcher
     * @param socketPath path to the socket
     * @param newConnectionCallback called on new connections
     */
    Acceptor(epoll::EventPoll& eventPoll,
             const std::string& socketPath,
             const NewConnectionCallback& newConnectionCallback);
    ~Acceptor();

    Acceptor(const Acceptor& acceptor) = delete;
    Acceptor& operator=(const Acceptor&) = delete;

private:
    epoll::EventPoll& mEventPoll;
    NewConnectionCallback mNewConnectionCallback;
    Socket mSocket;

    /**
     * Handle one incoming connection.
     * Used with external polling
     */
    void handleConnection();
};

} // namespace internals
} // namespace ipc
} // namespace cargo

#endif // CARGO_IPC_INTERNALS_ACCEPTOR_HPP
