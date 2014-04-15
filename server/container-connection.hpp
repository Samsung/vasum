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
 * @brief   Declaration of the class for communication between container and server
 */


#ifndef SERVER_CONTAINER_CONNECTION_HPP
#define SERVER_CONTAINER_CONNECTION_HPP

#include "dbus/connection.hpp"

#include <mutex>
#include <condition_variable>


namespace security_containers {


class ContainerConnection {

public:
    ContainerConnection();
    ~ContainerConnection();

    //TODO Move initialize, deinitialize and related stuff to separate class

    /**
     * Should be called every time just before container is created.
     */
    void initialize(const std::string& runMountPoint);

    /**
     * Should be called every time after container is destroyed.
     */
    void deinitialize();

    /**
     * Connect to container.
     */
    void connect();

    /**
     * Disconnect from container.
     */
    void disconnect();

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
    std::string mRunMountPoint;
    dbus::DbusConnection::Pointer mDbusConnection;
    std::mutex mNameMutex;
    std::condition_variable mNameCondition;
    bool mNameAcquired;
    bool mNameLost;
    NotifyActiveContainerCallback mNotifyActiveContainerCallback;

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


#endif // SERVER_CONTAINER_CONNECTION_HPP
