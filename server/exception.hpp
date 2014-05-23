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


namespace security_containers {


/**
 * Base class for exceptions in Security Containers Server
 */
struct ServerException: public SecurityContainersException {

    ServerException(const std::string& error = "") : SecurityContainersException(error) {}
};

/**
 * Error occurred during an attempt to perform an operation on a container,
 * e.g. start, stop a container
 */
struct ContainerOperationException: public ServerException {

    ContainerOperationException(const std::string& error = "") : ServerException(error) {}
};

/**
 * Exception during performing an operation on a container connection
 */
struct ContainerConnectionException: public ServerException {

    ContainerConnectionException(const std::string& error = "") : ServerException(error) {}
};

/**
* Exception during performing an operation by input monitor,
* e.g. create channel, register callback etc.
*/
struct InputMonitorException: public ServerException {

    InputMonitorException(const std::string& error = "") : ServerException(error) {}
};


}


#endif // SERVER_EXCEPTION_HPP
