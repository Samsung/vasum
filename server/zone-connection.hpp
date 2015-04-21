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
 * @brief   Declaration of a class for communication between zone and server
 */


#ifndef SERVER_ZONE_CONNECTION_HPP
#define SERVER_ZONE_CONNECTION_HPP

#include "dbus/connection.hpp"
#include "api/method-result-builder.hpp"

#include <mutex>
#include <condition_variable>

namespace vasum {


class ZoneConnection {

public:
    typedef std::function<void()> OnNameLostCallback;
    typedef std::function<void()> SwitchToDefaultCallback;

    ZoneConnection(const std::string& address, const OnNameLostCallback& callback);
    ~ZoneConnection();

    // ------------- API --------------

    typedef std::function<void(const std::string& application,
                               const std::string& message,
                               api::MethodResultBuilder::Pointer result
                              )> NotifyActiveZoneCallback;

    typedef std::function<void(const std::string& destination,
                               const std::string& path,
                               api::MethodResultBuilder::Pointer result
                              )> FileMoveCallback;

    typedef std::function<void(const std::string& target,
                               const std::string& targetBusName,
                               const std::string& targetObjectPath,
                               const std::string& targetInterface,
                               const std::string& targetMethod,
                               GVariant* parameters,
                               dbus::MethodResultBuilder::Pointer result
                              )> ProxyCallCallback;

    /**
     * Register notification request callback
     */
    void setNotifyActiveZoneCallback(const NotifyActiveZoneCallback& callback);

    /**
     * Register switch to default request callback
     */
    void setSwitchToDefaultCallback(const SwitchToDefaultCallback& callback);

    /*
     * Register file move request callback
     */
    void setFileMoveCallback(const FileMoveCallback& callback);

    /**
     * Register proxy call callback
     */
    void setProxyCallCallback(const ProxyCallCallback& callback);

    /**
     * Send notification signal to this zone
     */
    void sendNotification(const std::string& zone,
                          const std::string& application,
                          const std::string& message);

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
    OnNameLostCallback mOnNameLostCallback;
    NotifyActiveZoneCallback mNotifyActiveZoneCallback;
    SwitchToDefaultCallback mSwitchToDefaultCallback;
    FileMoveCallback mFileMoveCallback;
    ProxyCallCallback mProxyCallCallback;

    void onNameAcquired();
    void onNameLost();
    bool waitForNameAndSetCallback(const unsigned int timeoutMs, const OnNameLostCallback& callback);

    void onMessageCall(const std::string& objectPath,
                       const std::string& interface,
                       const std::string& methodName,
                       GVariant* parameters,
                       dbus::MethodResultBuilder::Pointer result);
    void onSignalReceived(const std::string& senderBusName,
                          const std::string& objectPath,
                          const std::string& interface,
                          const std::string& signalName,
                          GVariant* parameters);
};


} // namespace vasum


#endif // SERVER_ZONE_CONNECTION_HPP
