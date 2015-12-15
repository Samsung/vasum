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
 * @brief   Class for sending the result of a method
 */

#ifndef CARGO_IPC_METHOD_RESULT_HPP
#define CARGO_IPC_METHOD_RESULT_HPP

#include "cargo-ipc/types.hpp"
#include "logger/logger.hpp"
#include <memory>

namespace cargo {
namespace ipc {

namespace internals {

class Processor;

}

/**
 * Class used to obtain method call result code.
 * This class may be used to return value both:
 * - synchronously in the MethodHandler
 * - asynchronously, later outside MethodHandler
 *
 * @ingroup Types
 */
class MethodResult {
public:
    typedef std::shared_ptr<MethodResult> Pointer;

    MethodResult(internals::Processor& processor,
                 const MethodID methodID,
                 const MessageID& messageID,
                 const PeerID& peerID);


    template<typename Data>
    void set(const std::shared_ptr<Data>& data)
    {
        setInternal(data);
    }

    void setVoid();
    void setError(const int code, const std::string& message);
    PeerID getPeerID() const;

private:
    internals::Processor& mProcessor;
    MethodID mMethodID;
    PeerID mPeerID;
    MessageID mMessageID;

    void setInternal(const std::shared_ptr<void>& data);
};

template<typename SentDataType, typename ReceivedDataType>
struct MethodHandler {
    typedef std::function <HandlerExitCode(PeerID peerID,
                                           std::shared_ptr<ReceivedDataType>& data,
                                           MethodResult::Pointer methodResult) > type;
};

} // namespace ipc
} // namespace cargo

#endif // CARGO_IPC_METHOD_RESULT_HPP
