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
 * @brief   Declaration of a class for communication between container and server
 */


#ifndef SERVER_CONTAINER_CONNECTION_HPP
#define SERVER_CONTAINER_CONNECTION_HPP

#include "dbus/connection.hpp"

#include <mutex>
#include <condition_variable>


namespace security_containers {


class ContainerConnection {

public:
    typedef std::function<void()> OnNameLostCallback;

    ContainerConnection(const std::string& address, const OnNameLostCallback& callback);
    ~ContainerConnection();

    // ------------- API --------------

    typedef std::function<void(const std::string& application,
                               const std::string& message
                              )> NotifyActiveContainerCallback;

    /**
     * Register notification request callback
     */
    void setNotifyActiveContainerCallback(const NotifyActiveContainerCallback& callback);

    /**
     * Send notification signal to this container
     */
    void sendNotification(const std::string& container,
                          const std::string& application,
                          const std::string& message);

private:
    dbus::DbusConnection::Pointer mDbusConnection;
    std::mutex mNameMutex;
    std::condition_variable mNameCondition;
    bool mNameAcquired;
    bool mNameLost;
    OnNameLostCallback mOnNameLostCallback;
    NotifyActiveContainerCallback mNotifyActiveContainerCallback;

    void onNameAcquired();
    void onNameLost();
    bool waitForNameAndSetCallback(const unsigned int timeoutMs, const OnNameLostCallback& callback);

    void onMessageCall(const std::string& objectPath,
                       const std::string& interface,
                       const std::string& methodName,
                       GVariant* parameters,
                       dbus::MethodResultBuilder& result);
};


} // namespace security_containers


#endif // SERVER_CONTAINER_CONNECTION_HPP