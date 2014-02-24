/*
 *  Copyright (c) 2000 - 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Bumjin Im <bj.im@samsung.com>
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
 * @file    scs-exception.hpp
 * @author  Jan Olszak (j.olszak@samsung.com)
 * @brief   Exceptions for the server
 */


#ifndef SECURITY_CONTAINERS_SERVER_EXCEPTION_HPP
#define SECURITY_CONTAINERS_SERVER_EXCEPTION_HPP

#include <stdexcept>

namespace security_containers {

/**
 * @brief Base class for exceptions in Security Containers Server
 */
struct ServerException: public std::runtime_error {
    ServerException(const std::string& mess = "Security Containers Server Exception"):
        std::runtime_error(mess) {};
};

/**
 * @brief Error occured during an attempt to connect to libvirt's daemon.
 */
struct ConnectionException: public ServerException {
    ConnectionException(const std::string& mess = "Security Containers Connection Exception"):
        ServerException(mess) {};
};

/**
 * @brief Error occured during an attempt to perform an operation on a domain,
 * e.g. start, stop a container
 */
struct DomainOperationException: public ServerException {
    DomainOperationException(const std::string& mess = "Security Containers Domain Operation Exception"):
        ServerException(mess) {};
};

/**
 * @brief Error occured during a config file parsing,
 * e.g. syntax error
 */
struct ConfigException: public ServerException {
    ConfigException(const std::string& mess = "Security Containers Configuration Exception"):
        ServerException(mess) {};
};

}

#endif // SECURITY_CONTAINERS_SERVER_EXCEPTION_HPP
