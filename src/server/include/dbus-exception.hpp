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
 * @file    dbus-exception.hpp
 * @author  Piotr Bartosiewicz (p.bartosiewi@partner.samsung.com)
 * @brief   Dbus exceptions
 */


#ifndef DBUS_EXCEPTION_HPP
#define DBUS_EXCEPTION_HPP

#include <stdexcept>

/**
 * Base class for dbus exceptions
 */
struct DbusException: public std::runtime_error {
    using std::runtime_error::runtime_error;
};

/**
 * Dbus connection failed exception
 */
struct DbusConnectException: public DbusException {
    using DbusException::DbusException;
};

/**
 * Dbus operation failed exception
 * TODO split to more specific exceptions
 */
struct DbusOperationException: public DbusException {
    using DbusException::DbusException;
};

#endif // DBUS_EXCEPTION_HPP
