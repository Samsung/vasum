/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Piotr Bartosiewicz <p.bartosiewi@partner.samsung.com>
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
 * @author  Piotr Bartosiewicz (p.bartosiewi@partner.samsung.com)
 * @brief   Vasum base exception
 */


#ifndef COMMON_BASE_EXCEPTION_HPP
#define COMMON_BASE_EXCEPTION_HPP

#include <stdexcept>
#include <string>


namespace vasum {


/**
 * Base class vasum exceptions
 */
struct VasumException: public std::runtime_error {

    VasumException(const std::string& error = "") : std::runtime_error(error) {}
};

/**
 * Return string describing error number
 * it is wrapper for strerror_r
 */
std::string getSystemErrorMessage();
std::string getSystemErrorMessage(int err);

} // namespace vasum


#endif // COMMON_BASE_EXCEPTION_HPP
