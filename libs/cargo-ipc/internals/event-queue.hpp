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
 * @brief   Class for passing events using eventfd mechanism
 */

#ifndef CARGO_IPC_INTERNALS_EVENT_QUEUE_HPP
#define CARGO_IPC_INTERNALS_EVENT_QUEUE_HPP

#include "cargo-ipc/exception.hpp"
#include "utils/eventfd.hpp"
#include "logger/logger.hpp"

#include <string>
#include <mutex>
#include <queue>

namespace cargo {
namespace ipc {
namespace internals {


/**
 * This class implements a simple FIFO queue of events.
 * One can listen for the event with select/poll/epoll.
 *
 * @tparam MessageType type to pass as event's value
 */
template<typename MessageType>
class EventQueue {
public:
    EventQueue() = default;
    ~EventQueue() = default;
    EventQueue(const EventQueue& eventQueue) = delete;
    EventQueue& operator=(const EventQueue&) = delete;

    /**
     * @return reference to the event's file descriptor
     */
    int getFD() const;

    /**
     * Send an event
     *
     * @param message mesage to send
     */
    void send(const MessageType& message);

    /**
     * Receives the event.
     * Blocks if there is no event.
     *
     * @return event's value
     */
    MessageType receive();

    /**
     * @return is the queue empty
     */
    bool isEmpty();

private:
    typedef std::lock_guard<std::mutex> Lock;

    std::mutex mCommunicationMutex;
    std::queue<MessageType> mMessages;

    utils::EventFD mEventFD;
};

template<typename MessageType>
int EventQueue<MessageType>::getFD() const
{
    return mEventFD.getFD();
}

template<typename MessageType>
void EventQueue<MessageType>::send(const MessageType& mess)
{
    Lock lock(mCommunicationMutex);
    LOGT("Sending event");
    mMessages.push(mess);
    mEventFD.send();
}

template<typename MessageType>
MessageType EventQueue<MessageType>::receive()
{
    Lock lock(mCommunicationMutex);
    mEventFD.receive();
    LOGT("Received event");
    MessageType mess = mMessages.front();
    mMessages.pop();
    return mess;
}

template<typename MessageType>
bool EventQueue<MessageType>::isEmpty()
{
    Lock lock(mCommunicationMutex);
    return mMessages.empty();
}

} // namespace internals
} // namespace ipc
} // namespace cargo

#endif // CARGO_IPC_INTERNALS_EVENT_QUEUE_HPP
