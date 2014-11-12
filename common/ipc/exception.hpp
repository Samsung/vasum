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


#ifndef COMMON_IPC_EXCEPTION_HPP
#define COMMON_IPC_EXCEPTION_HPP

#include "base-exception.hpp"

namespace security_containers {


/**
 * Base class for exceptions in IPC
 */
struct IPCException: public SecurityContainersException {
    IPCException(const std::string& error) : SecurityContainersException(error) {}
};

struct IPCParsingException: public IPCException {
    IPCParsingException(const std::string& error) : IPCException(error) {}
};

struct IPCSerializationException: public IPCException {
    IPCSerializationException(const std::string& error) : IPCException(error) {}
};

struct IPCPeerDisconnectedException: public IPCException {
    IPCPeerDisconnectedException(const std::string& error) : IPCException(error) {}
};

struct IPCNaughtyPeerException: public IPCException {
    IPCNaughtyPeerException(const std::string& error) : IPCException(error) {}
};

struct IPCTimeoutException: public IPCException {
    IPCTimeoutException(const std::string& error) : IPCException(error) {}
};
}


#endif // COMMON_IPC_EXCEPTION_HPP
