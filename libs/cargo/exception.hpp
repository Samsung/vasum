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
 * @brief   Exceptions for libcargo
 */

#ifndef CARGO_EXCEPTION_HPP
#define CARGO_EXCEPTION_HPP

#include <stdexcept>

namespace cargo {

/**
 * Base class for exceptions in libcargo.
 */
struct CargoException: public std::runtime_error {

    CargoException(const std::string& error) : std::runtime_error(error) {}
};

/**
 * No such key error.
 */
struct NoKeyException: public CargoException {

    NoKeyException(const std::string& error) : CargoException(error) {}
};

/**
 * Invalid integral type integrity error.
 */
struct InternalIntegrityException: public CargoException {

    InternalIntegrityException(const std::string& error) : CargoException(error) {}
};

/**
 * Container size does not match.
 */
struct ContainerSizeException: public CargoException {

    ContainerSizeException(const std::string& error): CargoException(error) {}
};

} // namespace cargo

#endif // CARGO_EXCEPTION_HPP
