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
 * @file    base-exception.hpp
 * @author  Piotr Bartosiewicz (p.bartosiewi@partner.samsung.com)
 * @brief   Security containers base exception
 */


#ifndef BASE_EXCEPTION_HPP
#define BASE_EXCEPTION_HPP

#include <stdexcept>

namespace security_containers {

/**
 * Base class security containers exceptions
 */
struct SecurityContainersException: public std::runtime_error {
    using std::runtime_error::runtime_error;

    SecurityContainersException() : std::runtime_error(std::string()) {}
};

} // namespace security_containers

#endif // BASE_EXCEPTION_HPP
