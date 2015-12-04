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
 * @brief   Exceptions for the IPC
 */


#ifndef CARGO_IPC_EXCEPTION_HPP
#define CARGO_IPC_EXCEPTION_HPP

#include <stdexcept>

namespace cargo {
namespace ipc {

/**
 * @brief Base class for all exceptions in libcargo-ipc.
 * @ingroup Types
 * @defgroup IPCException   IPCException
 */
struct IPCException: public std::runtime_error {
    explicit IPCException(const std::string& message)
        : std::runtime_error(message) {}
};

/**
 * Exception to indicate error while reading/parsing data from the socket.
 * @ingroup IPCException
 */
struct IPCParsingException: public IPCException {
    explicit IPCParsingException(const std::string& message = "Exception during reading/parsing data from the socket")
        : IPCException(message) {}
};

/**
 * Exception to indicate error while writing/serializing data to the socket.
 * @ingroup IPCException
 */
struct IPCSerializationException: public IPCException {
    explicit IPCSerializationException(const std::string& message = "Exception during writing/serializing data to the socket")
        : IPCException(message) {}
};

/**
 * Exception to indicate that requested peer is not available, i.e. might got disconnected.
 * @ingroup IPCException
 */
struct IPCPeerDisconnectedException: public IPCException {
    explicit IPCPeerDisconnectedException(const std::string& message = "No such peer. Might got disconnected.")
        : IPCException(message) {}
};

/**
 * Exception to indicate that peer performed a forbidden action.
 * @ingroup IPCException
 */
struct IPCNaughtyPeerException: public IPCException {
    explicit IPCNaughtyPeerException(const std::string& message = "Peer performed a forbidden action.")
        : IPCException(message) {}
};

/**
 * Exception to indicate error while removing peer
 * @ingroup IPCException
 */
struct IPCRemovedPeerException: public IPCException {
    explicit IPCRemovedPeerException(const std::string& message = "Removing peer")
        : IPCException(message) {}
};

/**
 * Exception to indicate error while closing IPC channel
 * @ingroup IPCException
 */
struct IPCClosingException: public IPCException {
    explicit IPCClosingException(const std::string& message = "Closing IPC")
        : IPCException(message) {}
};

/**
 * Exception to indicate timeout event error
 * @ingroup IPCException
 */
struct IPCTimeoutException: public IPCException {
    explicit IPCTimeoutException(const std::string& message)
        : IPCException(message) {}
};

/**
 * Exception to indicate invalid result error
 * @ingroup IPCException
 */
struct IPCInvalidResultException: public IPCException {
    explicit IPCInvalidResultException(const std::string& message)
        : IPCException(message) {}
};

/**
 * Exception to indicate socket error
 * @ingroup IPCException
 */
struct IPCSocketException: public IPCException {
    IPCSocketException(const int code, const std::string& message)
        : IPCException(message),
          mCode(code)
    {}

    int getCode() const
    {
        return mCode;
    }

private:
    int mCode;
};

/**
 * Exception to indicate user error
 * @ingroup IPCException
 */
struct IPCUserException: public IPCException {
    IPCUserException(const int code, const std::string& message)
        : IPCException(message),
          mCode(code)
    {}

    int getCode() const
    {
        return mCode;
    }

private:
    int mCode;
};

} // namespace ipc
} // namespace cargo

#endif // CARGO_IPC_EXCEPTION_HPP
