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
 * @brief   Processor's request to call a method
 */

#ifndef COMMON_IPC_INTERNALS_METHOD_REQUEST_HPP
#define COMMON_IPC_INTERNALS_METHOD_REQUEST_HPP

#include "ipc/types.hpp"
#include "logger/logger-scope.hpp"
#include "config/manager.hpp"

namespace vasum {
namespace ipc {

class MethodRequest {
public:
    MethodRequest(const MethodRequest&) = delete;
    MethodRequest& operator=(const MethodRequest&) = delete;

    template<typename SentDataType, typename ReceivedDataType>
    static std::shared_ptr<MethodRequest> create(const MethodID methodID,
                                                 const FileDescriptor peerFD,
                                                 const std::shared_ptr<SentDataType>& data,
                                                 const typename ResultHandler<ReceivedDataType>::type& process);

    MethodID methodID;
    FileDescriptor peerFD;
    MessageID messageID;
    std::shared_ptr<void> data;
    SerializeCallback serialize;
    ParseCallback parse;
    ResultHandler<void>::type process;

private:
    MethodRequest(const MethodID methodID, const FileDescriptor peerFD)
        : methodID(methodID),
          peerFD(peerFD),
          messageID(getNextMessageID())
    {}
};


template<typename SentDataType, typename ReceivedDataType>
std::shared_ptr<MethodRequest> MethodRequest::create(const MethodID methodID,
                                                     const FileDescriptor peerFD,
                                                     const std::shared_ptr<SentDataType>& data,
                                                     const typename ResultHandler<ReceivedDataType>::type& process)
{
    std::shared_ptr<MethodRequest> request(new MethodRequest(methodID, peerFD));

    request->data = data;

    request->serialize = [](const int fd, std::shared_ptr<void>& data)->void {
        LOGS("Method serialize, peerFD: " << fd);
        config::saveToFD<SentDataType>(fd, *std::static_pointer_cast<SentDataType>(data));
    };

    request->parse = [](const int fd)->std::shared_ptr<void> {
        LOGS("Method parse, peerFD: " << fd);
        std::shared_ptr<ReceivedDataType> data(new ReceivedDataType());
        config::loadFromFD<ReceivedDataType>(fd, *data);
        return data;
    };

    request->process = [process](Status status, std::shared_ptr<void>& data)->void {
        LOGS("Method process, status: " << toString(status));
        std::shared_ptr<ReceivedDataType> tmpData = std::static_pointer_cast<ReceivedDataType>(data);
        return process(status, tmpData);
    };

    return request;
}

} // namespace ipc
} // namespace vasum

#endif // COMMON_IPC_INTERNALS_METHOD_REQUEST_HPP
