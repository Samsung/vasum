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
 * @file
 * @author  Piotr Bartosiewicz (p.bartosiewi@partner.samsung.com)
 * @brief   Example dbus api client
 */

#ifndef UNIT_TESTS_DBUS_TEST_CLIENT_HPP
#define UNIT_TESTS_DBUS_TEST_CLIENT_HPP

#include "dbus/connection.hpp" //TODO dbus/connection-iface.h

#include <string>
#include <memory>


namespace security_containers {


/**
 * Simple dbus client for test purposes.
 * Class used to test all possible kinds of dbus calls.
 */
class DbusTestClient {
public:
    DbusTestClient();

    // interface methods
    void noop();
    std::string process(const std::string& arg);
    void throwException(int arg);

private:
    dbus::DbusConnection::Pointer mConnection;
};


} // namespace security_containers


#endif // UNIT_TESTS_DBUS_TEST_CLIENT_HPP
