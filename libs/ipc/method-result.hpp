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

#ifndef COMMON_IPC_METHOD_RESULT_HPP
#define COMMON_IPC_METHOD_RESULT_HPP

#include "ipc/types.hpp"
#include "logger/logger.hpp"
#include <memory>

namespace ipc {

class Processor;

/**
 * @brief Class used to obtain method call result code.
 * @ingroup Types
 */
class MethodResult {
public:
    typedef std::shared_ptr<MethodResult> Pointer;

    MethodResult(Processor& processor,
                 const MethodID methodID,
                 const MessageID messageID,
                 const PeerID peerID);


    template<typename Data>
    void set(const std::shared_ptr<Data>& data)
    {
        setInternal(data);
    }

    void setVoid();
    void setError(const int code, const std::string& message);
    PeerID getPeerID();

private:
    Processor& mProcessor;
    MethodID mMethodID;
    PeerID mPeerID;
    MessageID mMessageID;

    void setInternal(const std::shared_ptr<void>& data);
};

template<typename SentDataType, typename ReceivedDataType>
struct MethodHandler {
    typedef std::function < void(PeerID peerID,
                                 std::shared_ptr<ReceivedDataType>& data,
                                 MethodResult::Pointer&& methodResult) > type;
};

} // namespace ipc

#endif // COMMON_IPC_METHOD_RESULT_HPP
