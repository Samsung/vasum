/*
 *  Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Mateusz Malicki <m.malicki2@samsung.com>
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
 * @author  Mateusz Malicki (m.malicki2@samsung.com)
 * @brief   Exceptions for the client
 */


#ifndef CLIENT_EXCEPTION_HPP
#define CLIENT_EXCEPTION_HPP

#include "base-exception.hpp"


namespace vasum {


/**
 * Base class for exceptions in Vasum Client
 */
struct ClientException: public VasumException {

    ClientException(const std::string& error) : VasumException(error) {}
};

struct IOException: public ClientException {

    IOException(const std::string& error) : ClientException(error) {}
};

struct OperationFailedException: public ClientException {

    OperationFailedException(const std::string& error) : ClientException(error) {}
};

struct InvalidArgumentException: public ClientException {

    InvalidArgumentException(const std::string& error) : ClientException(error) {}
};

struct InvalidResponseException: public ClientException {

    InvalidResponseException(const std::string& error) : ClientException(error) {}
};

}

#endif // CLIENT_EXCEPTION_HPP
