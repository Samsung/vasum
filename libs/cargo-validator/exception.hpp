/*
 *  Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Maciej Karpiuk (m.karpiuk2@samsung.com)
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
 * @author  Maciej Karpiuk (m.karpiuk2@samsung.com)
 * @brief   Exceptions for libcargo-validator
 */

#ifndef CARGO_VALIDATOR_EXCEPTION_HPP
#define CARGO_VALIDATOR_EXCEPTION_HPP

#include "cargo/exception.hpp"
#include <stdexcept>

namespace cargo {
namespace validator {
/**
 * Structure verification exception
 */
struct VerificationException: public CargoException {

    VerificationException(const std::string& error) : CargoException(error) {}
};

} // namespace validator
} // namespace cargo

#endif // CARGO_VALIDATOR_EXCEPTION_HPP
