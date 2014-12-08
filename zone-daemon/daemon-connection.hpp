/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Jan Olszak <j.olszak@samsung.com>
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
 * @author  Jan Olszak (j.olszak@samsung.com)
 * @brief   Declaration of a class for communication between zone and server
 */


#ifndef ZONE_DAEMON_DAEMON_CONNECTION_HPP
#define ZONE_DAEMON_DAEMON_CONNECTION_HPP

#include "dbus/connection.hpp"

#include <mutex>
#include <condition_variable>


namespace vasum {
namespace zone_daemon {


class DaemonConnection {

public:
    typedef std::function<void()> NameLostCallback;
    typedef std::function<void()> GainFocusCallback;
    typedef std::function<void()> LoseFocusCallback;

    DaemonConnection(const NameLostCallback&  nameLostCallback,
                     const GainFocusCallback& gainFocusCallback,
                     const LoseFocusCallback& loseFocusCallback);
    ~DaemonConnection();


private:
    dbus::DbusConnection::Pointer mDbusConnection;
    std::mutex mNameMutex;
    std::condition_variable mNameCondition;
    bool mNameAcquired;
    bool mNameLost;

    NameLostCallback  mNameLostCallback;
    GainFocusCallback mGainFocusCallback;
    LoseFocusCallback mLoseFocusCallback;

    void onNameAcquired();
    void onNameLost();
    bool waitForNameAndSetCallback(const unsigned int timeoutMs, const NameLostCallback& callback);

    void onMessageCall(const std::string& objectPath,
                       const std::string& interface,
                       const std::string& methodName,
                       GVariant* parameters,
                       dbus::MethodResultBuilder::Pointer result);
};


} // namespace zone_daemon
} // namespace vasum


#endif // ZONE_DAEMON_DAEMON_CONNECTION_HPP
