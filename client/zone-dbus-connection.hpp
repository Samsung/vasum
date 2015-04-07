/*
 *  Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
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
 * @brief   Zone client class
 */

#ifndef VASUM_CLIENT_ZONE_DBUS_CONNECTION_HPP
#define VASUM_CLIENT_ZONE_DBUS_CONNECTION_HPP

#include "dbus-connection.hpp"
#include <api/messages.hpp>

namespace vasum {
namespace client {

/**
 * vasum's client definition.
 *
 * ZoneDbusConnection is used for communication with the vasum's server from zone through dbus
 */
class ZoneDbusConnection {
public:
    typedef unsigned int SubscriptionId;
    typedef std::function<void(const vasum::api::Notification&)> NotificationCallback;

    ZoneDbusConnection();
    void create(const std::shared_ptr<dbus::DbusConnection>& connection);

    void callNotifyActiveZone(const vasum::api::NotifActiveZoneIn& argIn);
    void callFileMoveRequest(const vasum::api::FileMoveRequestIn& argIn,
                             vasum::api::FileMoveRequestStatus& argOut);
    SubscriptionId subscribeNotification(const NotificationCallback& callback);
    void unsubscribe(const SubscriptionId& id);
private:
    DbusConnection mConnection;
};

} // namespace client
} // namespace vasum

#endif /* VASUM_CLIENT_ZONE_DBUS_CONNECTION_HPP */
