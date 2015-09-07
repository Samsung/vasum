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
 * @brief   Exceptions for the server
 */


#ifndef SERVER_EXCEPTION_HPP
#define SERVER_EXCEPTION_HPP

#include "base-exception.hpp"


namespace vasum {


/**
 * Base class for exceptions in Vasum Server
 */
struct ServerException: public VasumException {

    explicit ServerException(const std::string& error) : VasumException(error) {}
};

/**
 * Error occurred during an attempt to perform an operation on a zone,
 * e.g. start, stop a zone
 */
struct ZoneOperationException: public ServerException {

    explicit ZoneOperationException(const std::string& error) : ServerException(error) {}
};

/**
 * Invalid zone id
 */
struct InvalidZoneIdException : public ServerException {

    explicit InvalidZoneIdException(const std::string& error) : ServerException(error) {}
};

/**
 * Exception during performing an operation on a zone connection
 */
struct ZoneConnectionException: public ServerException {

    explicit ZoneConnectionException(const std::string& error) : ServerException(error) {}
};

/**
 * Exception during performing an operation on a host connection
 */
struct HostConnectionException: public ServerException {

    explicit HostConnectionException(const std::string& error) : ServerException(error) {}
};

/**
* Exception during performing an operation by input monitor,
* e.g. create channel, register callback etc.
*/
struct InputMonitorException: public ServerException {

    explicit InputMonitorException(const std::string& error) : ServerException(error) {}
};

struct TimeoutException: public InputMonitorException {

    explicit TimeoutException(const std::string& error) : InputMonitorException (error) {}
};

}


#endif // SERVER_EXCEPTION_HPP
