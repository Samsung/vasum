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
 * @brief   Security containers base exception
 */


#ifndef COMMON_BASE_EXCEPTION_HPP
#define COMMON_BASE_EXCEPTION_HPP

#include <stdexcept>
#include <string>


namespace security_containers {


/**
 * Base class security containers exceptions
 */
struct SecurityContainersException: public std::runtime_error {

    SecurityContainersException(const std::string& error = "") : std::runtime_error(error) {}
};


} // namespace security_containers


#endif // COMMON_BASE_EXCEPTION_HPP
