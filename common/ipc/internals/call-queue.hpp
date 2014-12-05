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

#ifndef COMMON_IPC_INTERNALS_CALL_QUEUE_HPP
#define COMMON_IPC_INTERNALS_CALL_QUEUE_HPP

#include "ipc/types.hpp"
#include "config/manager.hpp"

#include <atomic>
#include <queue>

namespace vasum {
namespace ipc {

/**
* Class for managing a queue of calls in the Processor
*/
class CallQueue {
public:
    typedef std::function<void(int fd, std::shared_ptr<void>& data)> SerializeCallback;
    typedef std::function<std::shared_ptr<void>(int fd)> ParseCallback;

    struct Call {
        Call(const Call& other) = delete;
        Call& operator=(const Call&) = delete;
        Call() = default;
        Call(Call&&) = default;

        FileDescriptor peerFD;
        MethodID methodID;
        MessageID messageID;
        std::shared_ptr<void> data;
        SerializeCallback serialize;
        ParseCallback parse;
        ResultHandler<void>::type process;
    };

    CallQueue();
    ~CallQueue();

    CallQueue(const CallQueue&) = delete;
    CallQueue(CallQueue&&) = delete;
    CallQueue& operator=(const CallQueue&) = delete;

    template<typename SentDataType, typename ReceivedDataType>
    MessageID push(const MethodID methodID,
                   const FileDescriptor peerFD,
                   const std::shared_ptr<SentDataType>& data,
                   const typename ResultHandler<ReceivedDataType>::type& process);


    template<typename SentDataType>
    MessageID push(const MethodID methodID,
                   const FileDescriptor peerFD,
                   const std::shared_ptr<SentDataType>& data);

    Call pop();

    bool isEmpty() const;

private:
    std::queue<Call> mCalls;
    std::atomic<MessageID> mMessageIDCounter;

    MessageID getNextMessageID();
};


template<typename SentDataType, typename ReceivedDataType>
MessageID CallQueue::push(const MethodID methodID,
                          const FileDescriptor peerFD,
                          const std::shared_ptr<SentDataType>& data,
                          const typename ResultHandler<ReceivedDataType>::type& process)
{
    Call call;
    call.methodID = methodID;
    call.peerFD = peerFD;
    call.data = data;

    MessageID messageID = getNextMessageID();
    call.messageID = messageID;

    call.serialize = [](const int fd, std::shared_ptr<void>& data)->void {
        config::saveToFD<SentDataType>(fd, *std::static_pointer_cast<SentDataType>(data));
    };

    call.parse = [](const int fd)->std::shared_ptr<void> {
        std::shared_ptr<ReceivedDataType> data(new ReceivedDataType());
        config::loadFromFD<ReceivedDataType>(fd, *data);
        return data;
    };

    call.process = [process](Status status, std::shared_ptr<void>& data)->void {
        std::shared_ptr<ReceivedDataType> tmpData = std::static_pointer_cast<ReceivedDataType>(data);
        return process(status, tmpData);
    };

    mCalls.push(std::move(call));

    return messageID;
}

template<typename SentDataType>
MessageID CallQueue::push(const MethodID methodID,
                          const FileDescriptor peerFD,
                          const std::shared_ptr<SentDataType>& data)
{
    Call call;
    call.methodID = methodID;
    call.peerFD = peerFD;
    call.data = data;

    MessageID messageID = getNextMessageID();
    call.messageID = messageID;

    call.serialize = [](const int fd, std::shared_ptr<void>& data)->void {
        config::saveToFD<SentDataType>(fd, *std::static_pointer_cast<SentDataType>(data));
    };

    mCalls.push(std::move(call));

    return messageID;
}

} // namespace ipc
} // namespace vasum

#endif // COMMON_IPC_INTERNALS_CALL_QUEUE_HPP
