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
 * @brief   Declaration of a class for communication with server
 */


#ifndef SERVER_HOST_CONNECTION_HPP
#define SERVER_HOST_CONNECTION_HPP

#include "dbus/connection.hpp"

#include <mutex>
#include <condition_variable>


namespace security_containers {


class HostConnection {

public:
    HostConnection();
    ~HostConnection();

    // ------------- API --------------

    typedef std::function<void()> TestCallback;

    /**
     * Register test callback
     */
    void setTestCallback(const TestCallback& callback);

private:
    dbus::DbusConnection::Pointer mDbusConnection;
    std::mutex mNameMutex;
    std::condition_variable mNameCondition;
    bool mNameAcquired;
    bool mNameLost;
    TestCallback mTestCallback;

    void onNameAcquired();
    void onNameLost();
    bool waitForName(const unsigned int timeoutMs);

    void onMessageCall(const std::string& objectPath,
                       const std::string& interface,
                       const std::string& methodName,
                       GVariant* parameters,
                       dbus::MethodResultBuilder& result);
};


} // namespace security_containers


#endif // SERVER_HOST_CONNECTION_HPP
