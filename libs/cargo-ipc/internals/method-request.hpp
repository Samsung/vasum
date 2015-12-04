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

#ifndef CARGO_IPC_INTERNALS_METHOD_REQUEST_HPP
#define CARGO_IPC_INTERNALS_METHOD_REQUEST_HPP

#include "cargo-ipc/internals/result-builder.hpp"
#include "cargo-ipc/types.hpp"
#include "cargo-ipc/result.hpp"
#include "logger/logger-scope.hpp"
#include "cargo-fd/cargo-fd.hpp"
#include <utility>

namespace cargo {
namespace ipc {
namespace internals {

class MethodRequest {
public:
    MethodRequest(const MethodRequest&) = delete;
    MethodRequest& operator=(const MethodRequest&) = delete;

    template<typename SentDataType, typename ReceivedDataType>
    static std::shared_ptr<MethodRequest> create(const MethodID methodID,
                                                 const PeerID& peerID,
                                                 const std::shared_ptr<SentDataType>& data,
                                                 const typename ResultHandler<ReceivedDataType>::type& process);

    MethodID methodID;
    PeerID peerID;
    MessageID messageID;
    std::shared_ptr<void> data;
    SerializeCallback serialize;
    ParseCallback parse;
    ResultBuilderHandler process;

private:
    MethodRequest(const MethodID methodID, const PeerID& peerID)
        : methodID(methodID),
          peerID(peerID),
          messageID(getNextMessageID())
    {}
};


template<typename SentDataType, typename ReceivedDataType>
std::shared_ptr<MethodRequest> MethodRequest::create(const MethodID methodID,
                                                     const PeerID& peerID,
                                                     const std::shared_ptr<SentDataType>& data,
                                                     const typename ResultHandler<ReceivedDataType>::type& process)
{
    std::shared_ptr<MethodRequest> request(new MethodRequest(methodID, peerID));

    request->data = data;

    request->serialize = [](const int fd, std::shared_ptr<void>& data)->void {
        LOGS("Method serialize, peerID: " << fd);
        cargo::saveToFD<SentDataType>(fd, *std::static_pointer_cast<SentDataType>(data));
    };

    request->parse = [](const int fd)->std::shared_ptr<void> {
        LOGS("Method parse, peerID: " << fd);
        std::shared_ptr<ReceivedDataType> data(new ReceivedDataType());
        cargo::loadFromFD<ReceivedDataType>(fd, *data);
        return data;
    };

    if(process == nullptr){
        request->process = [](ResultBuilder & ) {
            LOGT("No method to process result");
        };
    } else {
        request->process = [process](ResultBuilder & resultBuilder) {
            LOGS("Method process");
            process(resultBuilder.build<ReceivedDataType>());
        };
    }

    return request;
}

} // namespace internals
} // namespace ipc
} // namespace cargo

#endif // CARGO_IPC_INTERNALS_METHOD_REQUEST_HPP
