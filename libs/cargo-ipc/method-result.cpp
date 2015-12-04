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

#include "config.hpp"

#include "cargo-ipc/method-result.hpp"
#include "cargo-ipc/internals/processor.hpp"

namespace cargo {
namespace ipc {

using namespace internals;

MethodResult::MethodResult(Processor& processor,
                           const MethodID methodID,
                           const MessageID& messageID,
                           const PeerID& peerID)
    : mProcessor(processor),
      mMethodID(methodID),
      mPeerID(peerID),
      mMessageID(messageID)
{}

void MethodResult::setInternal(const std::shared_ptr<void>& data)
{
    mProcessor.sendResult(mMethodID, mPeerID, mMessageID, data);
}

void MethodResult::setVoid()
{
    mProcessor.sendVoid(mMethodID, mPeerID, mMessageID);
}

void MethodResult::setError(const int code, const std::string& message)
{
    mProcessor.sendError(mPeerID, mMessageID, code, message);
}

PeerID MethodResult::getPeerID() const
{
    return mPeerID;
}

} // namespace ipc
} // namespace cargo
