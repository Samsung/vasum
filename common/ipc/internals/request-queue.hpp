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
 * @brief   Managing the queue of messages carrying any kind of data
 */

#ifndef COMMON_IPC_INTERNALS_MESSAGE_QUEUE_HPP
#define COMMON_IPC_INTERNALS_MESSAGE_QUEUE_HPP

#include "ipc/exception.hpp"
#include "ipc/internals/eventfd.hpp"
#include "logger/logger.hpp"

#include <list>
#include <memory>
#include <algorithm>

namespace vasum {
namespace ipc {

/**
* Class for managing a queue of Requests carrying any data
*/
template<typename RequestIdType>
class RequestQueue {
public:
    RequestQueue() = default;

    RequestQueue(const RequestQueue&) = delete;
    RequestQueue& operator=(const RequestQueue&) = delete;

    struct Request {
        Request(const Request& other) = delete;
        Request& operator=(const Request&) = delete;

        Request(Request&&) = default;
        Request(const RequestIdType requestID, const std::shared_ptr<void>& data)
            : requestID(requestID),
              data(data)
        {}

        template<typename DataType>
        std::shared_ptr<DataType> get()
        {
            return std::static_pointer_cast<DataType>(data);
        }

        RequestIdType requestID;
        std::shared_ptr<void> data;
    };

    /**
     * @return event's file descriptor
     */
    int getFD() const;

    /**
     * @return is the queue empty
     */
    bool isEmpty() const;

    /**
     * Push data to back of the queue
     *
     * @param requestID request type
     * @param data data corresponding to the request
     */
    void pushBack(const RequestIdType requestID,
                  const std::shared_ptr<void>& data = nullptr);

    /**
     * Push data to back of the queue
     *
     * @param requestID request type
     * @param data data corresponding to the request
     */
    void pushFront(const RequestIdType requestID,
                   const std::shared_ptr<void>& data = nullptr);

    /**
     * @return get the data from the next request
     */
    Request pop();

    /**
     * Remove elements from the queue when the predicate returns true
     *
     * @param predicate condition
     * @return was anything removed
     */
    template<typename Predicate>
    bool removeIf(Predicate predicate);

private:
    std::list<Request> mRequests;
    EventFD mEventFD;
};

template<typename RequestIdType>
int RequestQueue<RequestIdType>::getFD() const
{
    return mEventFD.getFD();
}

template<typename RequestIdType>
bool RequestQueue<RequestIdType>::isEmpty() const
{
    return mRequests.empty();
}

template<typename RequestIdType>
void RequestQueue<RequestIdType>::pushBack(const RequestIdType requestID,
                                           const std::shared_ptr<void>& data)
{
    Request request(requestID, data);
    mRequests.push_back(std::move(request));
    mEventFD.send();
}

template<typename RequestIdType>
void RequestQueue<RequestIdType>::pushFront(const RequestIdType requestID,
                                            const std::shared_ptr<void>& data)
{
    Request request(requestID, data);
    mRequests.push_front(std::move(request));
    mEventFD.send();
}

template<typename RequestIdType>
typename RequestQueue<RequestIdType>::Request RequestQueue<RequestIdType>::pop()
{
    mEventFD.receive();
    if (mRequests.empty()) {
        LOGE("Request queue is empty");
        throw IPCException("Request queue is empty");
    }
    Request request = std::move(mRequests.front());
    mRequests.pop_front();
    return request;
}

template<typename RequestIdType>
template<typename Predicate>
bool RequestQueue<RequestIdType>::removeIf(Predicate predicate)
{
    auto it = std::find_if(mRequests.begin(), mRequests.end(), predicate);
    if (it == mRequests.end()) {
        return false;
    }

    do {
        it = mRequests.erase(it);
        it = std::find_if(it, mRequests.end(), predicate);
    } while (it != mRequests.end());

    return true;
}
} // namespace ipc
} // namespace vasum

#endif // COMMON_IPC_INTERNALS_MESSAGE_QUEUE_HPP
