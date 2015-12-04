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
 * @brief   Processor's request to send the result of a method
 */

#ifndef CARGO_IPC_INTERNALS_SEND_RESULT_REQUEST_HPP
#define CARGO_IPC_INTERNALS_SEND_RESULT_REQUEST_HPP

#include "cargo-ipc/types.hpp"
#include "logger/logger-scope.hpp"

namespace cargo {
namespace ipc {
namespace internals {

class SendResultRequest {
public:
    SendResultRequest(const SendResultRequest&) = delete;
    SendResultRequest& operator=(const SendResultRequest&) = delete;

    SendResultRequest(const MethodID methodID,
                      const PeerID& peerID,
                      const MessageID& messageID,
                      const std::shared_ptr<void>& data)
        : methodID(methodID),
          peerID(peerID),
          messageID(messageID),
          data(data)
    {}

    MethodID methodID;
    PeerID peerID;
    MessageID messageID;
    std::shared_ptr<void> data;
};

} // namespace internals
} // namespace ipc
} // namespace cargo

#endif // CARGO_IPC_INTERNALS_SEND_RESULT_REQUEST_HPP
