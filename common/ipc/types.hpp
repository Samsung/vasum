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
 * @brief   Types definitions
 */

#ifndef COMMON_IPC_HANDLERS_HPP
#define COMMON_IPC_HANDLERS_HPP

#include "ipc/exception.hpp"

#include <functional>
#include <memory>
#include <string>

namespace vasum {
namespace ipc {

typedef int FileDescriptor;
typedef unsigned int MethodID;
typedef unsigned int MessageID;

typedef std::function<void(FileDescriptor)> PeerCallback;
typedef std::function<void(int fd, std::shared_ptr<void>& data)> SerializeCallback;
typedef std::function<std::shared_ptr<void>(int fd)> ParseCallback;

MessageID getNextMessageID();

template<typename SentDataType, typename ReceivedDataType>
struct MethodHandler {
    typedef std::function<std::shared_ptr<SentDataType>(FileDescriptor peerFD,
                                                        std::shared_ptr<ReceivedDataType>& data)> type;
};

template<typename ReceivedDataType>
struct SignalHandler {
    typedef std::function<void(FileDescriptor peerFD,
                               std::shared_ptr<ReceivedDataType>& data)> type;
};

} // namespace ipc
} // namespace vasum

#endif // COMMON_IPC_HANDLERS_HPP
