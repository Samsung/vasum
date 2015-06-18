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
 * @brief   Dbus API for the Zone Daemon
 */

#include "config.hpp"

#include "daemon-connection.hpp"
#include "daemon-dbus-definitions.hpp"
#include "exception.hpp"

#include "logger/logger.hpp"


namespace vasum {
namespace zone_daemon {

namespace {

// Timeout in ms for waiting for dbus name.
// Can happen if glib loop is busy or not present.
const unsigned int NAME_ACQUIRED_TIMEOUT = 5 * 1000;

} // namespace


DaemonConnection::DaemonConnection(const NameLostCallback& nameLostCallback,
                                   const GainFocusCallback& gainFocusCallback,
                                   const LoseFocusCallback& loseFocusCallback)
    : mNameAcquired(false)
    , mNameLost(false)
{
    LOGD("Connecting to DBUS on system bus");
    mDbusConnection = dbus::DbusConnection::createSystem();

    LOGD("Setting DBUS name");
    mDbusConnection->setName(zone_daemon::api::BUS_NAME,
                             std::bind(&DaemonConnection::onNameAcquired, this),
                             std::bind(&DaemonConnection::onNameLost, this));

    if (!waitForNameAndSetCallback(NAME_ACQUIRED_TIMEOUT, nameLostCallback)) {
        LOGE("Could not acquire dbus name: " << zone_daemon::api::BUS_NAME);
        throw ZoneDaemonException("Could not acquire dbus name: " + zone_daemon::api::BUS_NAME);
    }

    LOGD("Setting callbacks");
    mGainFocusCallback = gainFocusCallback;
    mLoseFocusCallback = loseFocusCallback;


    LOGD("Registering DBUS interface");
    using namespace std::placeholders;
    mDbusConnection->registerObject(zone_daemon::api::OBJECT_PATH,
                                    zone_daemon::api::DEFINITION,
                                    std::bind(&DaemonConnection::onMessageCall,
                                              this, _1, _2, _3, _4, _5),
                                    nullptr);
    LOGD("Connected");
}

DaemonConnection::~DaemonConnection()
{
}

bool DaemonConnection::waitForNameAndSetCallback(const unsigned int timeoutMs,
                                                 const NameLostCallback& nameLostCallback)
{
    std::unique_lock<std::mutex> lock(mNameMutex);
    mNameCondition.wait_for(lock,
                            std::chrono::milliseconds(timeoutMs),
    [this] {
        return mNameAcquired || mNameLost;
    });

    if (mNameAcquired) {
        mNameLostCallback = nameLostCallback;
    }

    return mNameAcquired;
}

void DaemonConnection::onNameAcquired()
{
    std::unique_lock<std::mutex> lock(mNameMutex);
    mNameAcquired = true;
    mNameCondition.notify_one();
}

void DaemonConnection::onNameLost()
{
    std::unique_lock<std::mutex> lock(mNameMutex);
    mNameLost = true;
    mNameCondition.notify_one();

    if (mNameLostCallback) {
        mNameLostCallback();
    }
}

void DaemonConnection::onMessageCall(const std::string& objectPath,
                                     const std::string& interface,
                                     const std::string& methodName,
                                     GVariant* /*parameters*/,
                                     dbus::MethodResultBuilder::Pointer result)
{
    if (objectPath != api::OBJECT_PATH || interface != api::INTERFACE) {
        return;
    }

    if (methodName == api::METHOD_GAIN_FOCUS) {
        if (mGainFocusCallback) {
            mGainFocusCallback();
            result->setVoid();
        }
    } else if (methodName == api::METHOD_LOSE_FOCUS) {
        if (mLoseFocusCallback) {
            mLoseFocusCallback();
            result->setVoid();
        }
    }
}

} // namespace zone_daemon
} // namespace vasum
