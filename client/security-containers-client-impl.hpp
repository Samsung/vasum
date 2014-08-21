/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Mateusz Malicki <m.malicki2@samsung.com>
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
 * @author  Mateusz Malicki (m.malicki2@samsung.com)
 * @brief   This file contains security-containers-server's client definition
 */

#ifndef SECURITY_CONTAINERS_CLIENT_IMPL_HPP
#define SECURITY_CONTAINERS_CLIENT_IMPL_HPP

#include "security-containers-client.h"
#include <dbus/connection.hpp>
#include <exception>

/**
 * Structure which defines the dbus interface.
 */
struct DbusInterfaceInfo {
    DbusInterfaceInfo(const std::string& busName,
                      const std::string& objectPath,
                      const std::string& interface)
        : busName(busName),
          objectPath(objectPath),
          interface(interface) {}
    const std::string busName;
    const std::string objectPath;
    const std::string interface;
};

/**
 * security-containers's client definition.
 *
 * Client uses dbus API.
 */
class Client {
private:
    typedef std::function<void(GVariant* parameters)> SignalCallback;
    struct Status {
        Status();
        Status(ScStatus status, const std::string& msg);
        ScStatus mScStatus;
        std::string mMsg;
    };

    dbus::DbusConnection::Pointer mConnection;
    Status mStatus;

    ScStatus callMethod(const DbusInterfaceInfo& info,
                        const std::string& method,
                        GVariant* args_in,
                        const std::string& args_spec_out = std::string(),
                        GVariant** args_out = NULL);
    ScStatus signalSubscribe(const DbusInterfaceInfo& info,
                             const std::string& name,
                             SignalCallback signalCallback);

public:
    Client() noexcept;
    ~Client() noexcept;

    /**
     * Create client with system dbus address.
     *
     * @return status of this function call
     */
    ScStatus createSystem() noexcept;

    /**
     * Create client.
     *
     * @param address Dbus socket address
     * @return status of this function call
     */
    ScStatus create(const std::string& address) noexcept;

    /**
     *  @see ::sc_get_status_message
     */
    const char* sc_get_status_message() noexcept;

    /**
     *  @see ::sc_get_status
     */
    ScStatus sc_get_status() noexcept;

    /**
     *  @see ::sc_get_container_dbuses
     */
    ScStatus sc_get_container_dbuses(ScArrayString* keys, ScArrayString* values) noexcept;

    /**
     *  @see ::sc_get_container_ids
     */
    ScStatus sc_get_container_ids(ScArrayString* array) noexcept;

    /**
     *  @see ::sc_get_active_container_id
     */
    ScStatus sc_get_active_container_id(ScString* id) noexcept;

    /**
     *  @see ::sc_set_active_container
     */
    ScStatus sc_set_active_container(const char* id) noexcept;

    /**
     *  @see ::sc_container_dbus_state
     */
    ScStatus sc_container_dbus_state(ScContainerDbusStateCallback containerDbusStateCallback) noexcept;

    /**
     *  @see ::sc_notify_active_container
     */
    ScStatus sc_notify_active_container(const char* application, const char* message) noexcept;

    /**
     *  @see ::sc_notification
     */
    ScStatus sc_notification(ScNotificationCallback notificationCallback) noexcept;
    /**
     *  @see ::sc_start_glib_loop
     */
    static ScStatus sc_start_glib_loop() noexcept;

    /**
     *  @see ::sc_stop_glib_loop
     */
    static ScStatus sc_stop_glib_loop() noexcept;
};

#endif /* SECURITY_CONTAINERS_CLIENT_IMPL_HPP */
