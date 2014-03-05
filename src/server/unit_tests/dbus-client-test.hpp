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
 * @file    dbus-client-test.hpp
 * @author  Piotr Bartosiewicz (p.bartosiewi@partner.samsung.com)
 * @brief   Example dbus api client
 */

#ifndef DBUS_CLIENT_TEST_HPP
#define DBUS_CLIENT_TEST_HPP

#include <string>
#include <memory>

class DbusConnection;
typedef std::shared_ptr<DbusConnection> DbusConnectionPtr;//TODO include dbus-connection-iface.h

/**
 * Simple dbus client for test purposes.
 * Class used to test all possible kinds of dbus calls.
 */
class DbusClientTest {
public:
    DbusClientTest();

    // interface methods
    void noop();
    std::string process(const std::string& arg);
    void throwException(int arg);

private:
    DbusConnectionPtr mConnection;
};

#endif //DBUS_CLIENT_TEST_HPP
