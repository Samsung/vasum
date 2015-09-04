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

#ifndef COMMON_IPC_TYPES_HPP
#define COMMON_IPC_TYPES_HPP

#include <functional>
#include <memory>
#include <string>

namespace ipc {

/**
 * @brief Generic types used in libIpc.
 *
 * @ingroup libIpc
 * @defgroup Types libIpc tools
 */

typedef int FileDescriptor;
typedef unsigned int MethodID;
typedef std::string MessageID;
typedef std::string PeerID;

/**
 * Generic function type used as callback for peer events.
 *
 * @param   peerID          peer identifier that event relates to
 * @param   fd              event origin
 * @ingroup Types
 */
typedef std::function<void(const ipc::PeerID peerID, const ipc::FileDescriptor fd)> PeerCallback;

/**
 * Generic function type used as callback for serializing and
 * saving serialized data to the descriptor.
 *
 * @param   fd              descriptor to save the serialized data to
 * @param   data            data to serialize
 * @ingroup Types
 */
typedef std::function<void(ipc::FileDescriptor fd, std::shared_ptr<void>& data)> SerializeCallback;

/**
 * Generic function type used as callback for reading and parsing data.
 *
 * @param   fd              descriptor to read the data from
 * @ingroup Types
 */
typedef std::function<std::shared_ptr<void>(ipc::FileDescriptor fd)> ParseCallback;

/**
 * Generate an unique message id.
 *
 * @return new, unique MessageID
 * @ingroup Types
 */
MessageID getNextMessageID();

/**
 * Generate an unique peer id.
 *
 * @return new, unique PeerID
 * @ingroup Types
 */
PeerID getNextPeerID();


/**
 * Generic type used as a callback function for handling signals.
 * @tparam ReceivedDataType     type of received data
 * @ingroup Types
 */
template<typename ReceivedDataType>
struct SignalHandler {
    typedef std::function<void(PeerID peerID,
                               std::shared_ptr<ReceivedDataType>& data)> type;
};

} // namespace ipc

#endif // COMMON_IPC_TYPES_HPP
