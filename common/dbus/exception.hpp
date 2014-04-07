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


#ifndef COMMON_DBUS_EXCEPTION_HPP
#define COMMON_DBUS_EXCEPTION_HPP

#include "base-exception.hpp"


namespace security_containers {
namespace dbus {


/**
 * Base class for dbus exceptions
 */
struct DbusException: public SecurityContainersException {
    using SecurityContainersException::SecurityContainersException;
};

/**
 * Dbus IO exception (connection failed, connection lost, etc)
 */
struct DbusIOException: public DbusException {
    using DbusException::DbusException;
};

/**
 * Dbus operation failed exception
 */
struct DbusOperationException: public DbusException {
    using DbusException::DbusException;
};

/**
 * Dbus custom exception triggered by user logic
 */
struct DbusCustomException: public DbusException {
    using DbusException::DbusException;
};

/**
 * Dbus invalid argument exception
 */
struct DbusInvalidArgumentException: public DbusException {
    using DbusException::DbusException;
};


} // namespace dbus
} // namespace security_containers


#endif // COMMON_DBUS_EXCEPTION_HPP
