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
 * @brief   Processor's request to send a signal
 */

#ifndef CARGO_IPC_INTERNALS_SIGNAL_REQUEST_HPP
#define CARGO_IPC_INTERNALS_SIGNAL_REQUEST_HPP

#include "cargo-ipc/types.hpp"
#include "logger/logger-scope.hpp"

namespace cargo {
namespace ipc {
namespace internals {

class SignalRequest {
public:
    SignalRequest(const SignalRequest&) = delete;
    SignalRequest& operator=(const SignalRequest&) = delete;

    template<typename SentDataType>
    static std::shared_ptr<SignalRequest> create(const MethodID methodID,
                                                 const PeerID& peerID,
                                                 const std::shared_ptr<SentDataType>& data);

    MethodID methodID;
    PeerID peerID;
    MessageID messageID;
    std::shared_ptr<void> data;
    SerializeCallback serialize;

private:
    SignalRequest(const MethodID methodID, const PeerID& peerID)
        : methodID(methodID),
          peerID(peerID),
          messageID(getNextMessageID())
    {}

};

template<typename SentDataType>
std::shared_ptr<SignalRequest> SignalRequest::create(const MethodID methodID,
                                                     const PeerID& peerID,
                                                     const std::shared_ptr<SentDataType>& data)
{
    std::shared_ptr<SignalRequest> request(new SignalRequest(methodID, peerID));

    request->data = data;

    request->serialize = [](const int fd, std::shared_ptr<void>& data)->void {
        LOGS("Signal serialize, peerID: " << fd);
        cargo::saveToFD<SentDataType>(fd, *std::static_pointer_cast<SentDataType>(data));
    };

    return request;
}

} // namespace internals
} // namespace ipc
} // namespace cargo

#endif // CARGO_IPC_INTERNALS_SIGNAL_REQUEST_HPP
