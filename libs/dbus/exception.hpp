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
 * @brief   Dbus exceptions
 */

#ifndef DBUS_EXCEPTION_HPP
#define DBUS_EXCEPTION_HPP

#include <stdexcept>

namespace dbus {

/**
 * Base class for dbus exceptions
 */
struct DbusException: public std::runtime_error {

    explicit DbusException(const std::string& error = "") : std::runtime_error(error) {}
};

/**
 * Dbus IO exception (connection failed, connection lost, etc)
 */
struct DbusIOException: public DbusException {

    explicit DbusIOException(const std::string& error = "") : DbusException(error) {}
};

/**
 * Dbus operation failed exception
 */
struct DbusOperationException: public DbusException {

    explicit DbusOperationException(const std::string& error = "") : DbusException(error) {}
};

/**
 * Dbus custom exception triggered by user logic
 */
struct DbusCustomException: public DbusException {

    explicit DbusCustomException(const std::string& error = "") : DbusException(error) {}
};

/**
 * Dbus invalid argument exception
 */
struct DbusInvalidArgumentException: public DbusException {

    explicit DbusInvalidArgumentException(const std::string& error = "") : DbusException(error) {}
};

} // namespace dbus

#endif // DBUS_EXCEPTION_HPP
