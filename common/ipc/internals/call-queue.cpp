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
 * @brief   Managing the queue with calls
 */

#include "config.hpp"

#include "ipc/internals/call-queue.hpp"
#include "ipc/exception.hpp"
#include "logger/logger.hpp"

namespace security_containers {
namespace ipc {

CallQueue::CallQueue()
    : mMessageIDCounter(0)
{
}

CallQueue::~CallQueue()
{
}

bool CallQueue::isEmpty() const
{
    return mCalls.empty();
}

MessageID CallQueue::getNextMessageID()
{
    // TODO: This method of generating UIDs is buggy. To be changed.
    return ++mMessageIDCounter;
}

CallQueue::Call CallQueue::pop()
{
    if (isEmpty()) {
        LOGE("CallQueue is empty");
        throw IPCException("CallQueue is empty");
    }
    Call call = std::move(mCalls.front());
    mCalls.pop();
    return call;
}

} // namespace ipc
} // namespace security_containers
