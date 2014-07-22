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

    typedef std::function<void(const std::string& target,
                               const std::string& targetBusName,
                               const std::string& targetObjectPath,
                               const std::string& targetInterface,
                               const std::string& targetMethod,
                               GVariant* parameters,
                               dbus::MethodResultBuilder::Pointer result
                              )> ProxyCallCallback;

    typedef std::function<void(dbus::MethodResultBuilder::Pointer result
                              )> GetContainerDbusesCallback;
    typedef std::function<void(dbus::MethodResultBuilder::Pointer result)> GetContainerIdsCallback;
    typedef std::function<void(dbus::MethodResultBuilder::Pointer result
                              )> GetActiveContainerIdCallback;


    /**
     * Register proxy call callback
     */
    void setProxyCallCallback(const ProxyCallCallback& callback);

    /**
     * Register get container dbuses callback
     */
    void setGetContainerDbusesCallback(const GetContainerDbusesCallback& callback);

    /**
     * Send signal describing dbus address state change
     */
    void signalContainerDbusState(const std::string& containerId, const std::string& dbusAddress);

    /**
     * Register a callback called to get a list of domain ids
     */
    void setGetContainerIdsCallback(const GetContainerIdsCallback& callback);

    /**
     * Register a callback called to get the active container id
     */
    void setGetActiveContainerIdCallback(const GetContainerIdsCallback& callback);

    /**
     * Make a proxy call
     */
    void proxyCallAsync(const std::string& busName,
                        const std::string& objectPath,
                        const std::string& interface,
                        const std::string& method,
                        GVariant* parameters,
                        const dbus::DbusConnection::AsyncMethodCallCallback& callback);

private:
    dbus::DbusConnection::Pointer mDbusConnection;
    std::mutex mNameMutex;
    std::condition_variable mNameCondition;
    bool mNameAcquired;
    bool mNameLost;
    ProxyCallCallback mProxyCallCallback;
    GetContainerDbusesCallback mGetContainerDbusesCallback;
    GetContainerIdsCallback mGetContainerIdsCallback;
    GetActiveContainerIdCallback mGetActiveContainerIdCallback;

    void onNameAcquired();
    void onNameLost();
    bool waitForName(const unsigned int timeoutMs);

    void onMessageCall(const std::string& objectPath,
                       const std::string& interface,
                       const std::string& methodName,
                       GVariant* parameters,
                       dbus::MethodResultBuilder::Pointer result);
};


} // namespace security_containers


#endif // SERVER_HOST_CONNECTION_HPP
