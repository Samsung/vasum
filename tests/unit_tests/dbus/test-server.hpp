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
 * @brief   Example dbus api server
 */

#ifndef UNIT_TESTS_DBUS_TEST_SERVER_HPP
#define UNIT_TESTS_DBUS_TEST_SERVER_HPP

#include "dbus/connection.hpp"

#include <string>
#include <memory>
#include <mutex>
#include <condition_variable>


namespace vasum {


/**
 * Simple dbus server for test purposes.
 * Class used to test all possible kinds of callbacks.
 */
class DbusTestServer {
public:
    DbusTestServer();

    typedef std::function<void()> DisconnectCallback;
    void setDisconnectCallback(const DisconnectCallback& callback);

    void notifyClients(const std::string& message);

private:
    //{ interface methods
    void noop();
    std::string process(const std::string& arg);
    void throwException(int arg);
    //}

    dbus::DbusConnection::Pointer mConnection;
    DisconnectCallback mDisconnectCallback;
    bool mNameAcquired;
    bool mPendingDisconnect;
    std::mutex mMutex;
    std::condition_variable mNameCondition;


    bool waitForName();

    void onNameAcquired();
    void onDisconnect();

    void onMessageCall(const std::string& objectPath,
                       const std::string& interface,
                       const std::string& methodName,
                       GVariant* parameters,
                       dbus::MethodResultBuilder::Pointer result);
};


} // namespace vasum


#endif // UNIT_TESTS_DBUS_TEST_SERVER_HPP
