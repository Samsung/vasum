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

#include <config.hpp>
#include "zone-dbus-connection.hpp"
#include <api/messages.hpp>
#include <zone-dbus-definitions.hpp>

namespace vasum {
namespace client {

ZoneDbusConnection::ZoneDbusConnection()
    : mConnection(vasum::api::zone::DEFINITION,
                  vasum::api::zone::BUS_NAME,
                  vasum::api::zone::OBJECT_PATH,
                  vasum::api::zone::INTERFACE)
{
}

void ZoneDbusConnection::create(const std::shared_ptr<dbus::DbusConnection>& connection)
{
    mConnection.create(connection);
}

void ZoneDbusConnection::callNotifyActiveZone(const vasum::api::NotifActiveZoneIn& argIn)
{
    mConnection.call(vasum::api::zone::METHOD_NOTIFY_ACTIVE_ZONE, argIn);
}

void ZoneDbusConnection::callFileMoveRequest(const vasum::api::FileMoveRequestIn& argIn,
                                             vasum::api::FileMoveRequestStatus& argOut)
{
    mConnection.call(vasum::api::zone::METHOD_FILE_MOVE_REQUEST, argIn, argOut);
}

ZoneDbusConnection::SubscriptionId
ZoneDbusConnection::subscribeNotification(const NotificationCallback& callback)
{
    return mConnection.signalSubscribe<vasum::api::Notification>(
        vasum::api::zone::SIGNAL_NOTIFICATION, callback);
}

void ZoneDbusConnection::unsubscribe(const SubscriptionId& id )
{
    mConnection.signalUnsubscribe(id);
}

} // namespace client
} // namespace vasum
