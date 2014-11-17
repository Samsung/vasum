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
 * @brief   Types definitions and helper functions
 */

#include "ipc/types.hpp"
#include "logger/logger.hpp"


namespace security_containers {
namespace ipc {

std::string toString(const Status status)
{
    switch (status) {
    case Status::OK: return "No error, everything is OK";
    case Status::PARSING_ERROR: return "Exception during reading/parsing data from the socket";
    case Status::SERIALIZATION_ERROR: return "Exception during writing/serializing data to the socket";
    case Status::PEER_DISCONNECTED: return "No such peer. Might got disconnected.";
    case Status::NAUGHTY_PEER: return "Peer performed a forbidden action.";
    case Status::REMOVED_PEER: return "Removing peer";
    case Status::CLOSING: return "Closing IPC";
    case Status::UNDEFINED: return "Undefined state";
    default: return "Unknown status";
    }
}

void throwOnError(const Status status)
{
    if (status == Status::OK) {
        return;
    }

    std::string message = toString(status);
    LOGE(message);

    switch (status) {
    case Status::PARSING_ERROR: throw IPCParsingException(message);
    case Status::SERIALIZATION_ERROR: throw IPCSerializationException(message);
    case Status::PEER_DISCONNECTED: throw IPCPeerDisconnectedException(message);
    case Status::NAUGHTY_PEER: throw IPCNaughtyPeerException(message);
    case Status::REMOVED_PEER: throw IPCException(message);
    case Status::CLOSING: throw IPCException(message);
    case Status::UNDEFINED: throw IPCException(message);
    default: return throw IPCException(message);
    }
}
} // namespace ipc
} // namespace security_containers
