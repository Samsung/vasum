/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Lukasz Pawelczyk <l.pawelczyk@partner.samsung.com>
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
 * @author  Lukasz Pawelczyk (l.pawelczyk@partner.samsung.com)
 * @brief   Exceptions for the server
 */


#ifndef COMMON_UTILS_EXCEPTION_HPP
#define COMMON_UTILS_EXCEPTION_HPP

#include <stdexcept>

namespace utils {


/**
 * Base class for exceptions in utils
 */
struct UtilsException: public std::runtime_error {

    explicit UtilsException(const std::string& error) : std::runtime_error(error) {}
};

struct EventFDException: public UtilsException {

    explicit EventFDException(const std::string& error) : UtilsException(error) {}
};

struct ProvisionExistsException: public UtilsException {

    explicit ProvisionExistsException(const std::string& error) : UtilsException(error) {}
};

/**
 * Return string describing error number
 * it is wrapper for strerror_r
 */
std::string getSystemErrorMessage();
std::string getSystemErrorMessage(int err);

} // namespace utils

#endif // COMMON_UTILS_EXCEPTION_HPP
