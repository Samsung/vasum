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
 * @brief   Implementation of a class for communication between zone and server
 */

#include "config.hpp"

#include "zone-connection.hpp"
#include "zone-dbus-definitions.hpp"
#include "exception.hpp"
// TODO: Switch to real power-manager dbus defs when they will be implemented in power-manager
#include "fake-power-manager-dbus-definitions.hpp"

#include "api/dbus-method-result-builder.hpp"
#include "api/messages.hpp"

#include "logger/logger.hpp"


namespace vasum {

namespace {

// Timeout in ms for waiting for dbus name.
// Can happen if glib loop is busy or not present.
// TODO: this should be in zone's configuration file
const unsigned int NAME_ACQUIRED_TIMEOUT = 5 * 1000;

} // namespace


ZoneConnection::ZoneConnection(const std::string& address, const OnNameLostCallback& callback)
    : mNameAcquired(false)
    , mNameLost(false)
{
    if (address.empty()) {
        LOGE("Invalid zone connection address");
        throw ZoneConnectionException("Invalid zone connection address");
    }

    LOGT("Connecting to DBUS on " << address);
    mDbusConnection = dbus::DbusConnection::create(address);

    LOGT("Setting DBUS name");
    mDbusConnection->setName(api::zone::BUS_NAME,
                             std::bind(&ZoneConnection::onNameAcquired, this),
                             std::bind(&ZoneConnection::onNameLost, this));

    if (!waitForNameAndSetCallback(NAME_ACQUIRED_TIMEOUT, callback)) {
        LOGE("Could not acquire dbus name: " << api::zone::BUS_NAME);
        throw ZoneConnectionException("Could not acquire dbus name: " + api::zone::BUS_NAME);
    }

    LOGT("Registering DBUS interface");
    using namespace std::placeholders;
    mDbusConnection->registerObject(api::zone::OBJECT_PATH,
                                    api::zone::DEFINITION,
                                    std::bind(&ZoneConnection::onMessageCall,
                                              this,
                                              _1,
                                              _2,
                                              _3,
                                              _4,
                                              _5));

    mDbusConnection->signalSubscribe(std::bind(&ZoneConnection::onSignalReceived,
                                               this,
                                               _1,
                                               _2,
                                               _3,
                                               _4,
                                               _5),
                                     std::string(fake_power_manager_api::BUS_NAME));

    LOGD("Connected");
}

ZoneConnection::~ZoneConnection()
{
}

bool ZoneConnection::waitForNameAndSetCallback(const unsigned int timeoutMs, const OnNameLostCallback& callback)
{
    std::unique_lock<std::mutex> lock(mNameMutex);
    mNameCondition.wait_for(lock,
                            std::chrono::milliseconds(timeoutMs),
                            [this] {
                                return mNameAcquired || mNameLost;
                            });
    if(mNameAcquired) {
        mOnNameLostCallback = callback;
    }

    return mNameAcquired;
}

void ZoneConnection::onNameAcquired()
{
    std::unique_lock<std::mutex> lock(mNameMutex);
    mNameAcquired = true;
    mNameCondition.notify_one();
}

void ZoneConnection::onNameLost()
{
    std::unique_lock<std::mutex> lock(mNameMutex);
    mNameLost = true;
    mNameCondition.notify_one();

    if (mOnNameLostCallback) {
        mOnNameLostCallback();
    }
}

void ZoneConnection::setNotifyActiveZoneCallback(
    const NotifyActiveZoneCallback& callback)
{
    mNotifyActiveZoneCallback = callback;
}

void ZoneConnection::setSwitchToDefaultCallback(const SwitchToDefaultCallback& callback)
{
    mSwitchToDefaultCallback = callback;
}

void ZoneConnection::setFileMoveCallback(
    const FileMoveCallback& callback)
{
    mFileMoveCallback = callback;
}

void ZoneConnection::setProxyCallCallback(const ProxyCallCallback& callback)
{
    mProxyCallCallback = callback;
}

void ZoneConnection::onMessageCall(const std::string& objectPath,
                                        const std::string& interface,
                                        const std::string& methodName,
                                        GVariant* parameters,
                                        dbus::MethodResultBuilder::Pointer result)
{
    if (objectPath != api::zone::OBJECT_PATH || interface != api::zone::INTERFACE) {
        return;
    }

    if (methodName == api::zone::METHOD_NOTIFY_ACTIVE_ZONE) {
        const gchar* application = NULL;
        const gchar* message = NULL;
        g_variant_get(parameters, "(&s&s)", &application, &message);
        if (mNotifyActiveZoneCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::Void>>(result);
            mNotifyActiveZoneCallback(application, message, rb);
        }
    }

    if (methodName == api::zone::METHOD_FILE_MOVE_REQUEST) {
        const gchar* destination = NULL;
        const gchar* path = NULL;
        g_variant_get(parameters, "(&s&s)", &destination, &path);
        if (mFileMoveCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::FileMoveRequestStatus>>(result);
            mFileMoveCallback(destination, path, rb);
        }
    }

    if (methodName == api::METHOD_PROXY_CALL) {
        const gchar* target = NULL;
        const gchar* targetBusName = NULL;
        const gchar* targetObjectPath = NULL;
        const gchar* targetInterface = NULL;
        const gchar* targetMethod = NULL;
        GVariant* rawArgs = NULL;
        g_variant_get(parameters,
                      "(&s&s&s&s&sv)",
                      &target,
                      &targetBusName,
                      &targetObjectPath,
                      &targetInterface,
                      &targetMethod,
                      &rawArgs);
        dbus::GVariantPtr args(rawArgs, g_variant_unref);

        if (mProxyCallCallback) {
            mProxyCallCallback(target,
                               targetBusName,
                               targetObjectPath,
                               targetInterface,
                               targetMethod,
                               args.get(),
                               result);
        }
    }
}

void ZoneConnection::onSignalReceived(const std::string& senderBusName,
                                           const std::string& objectPath,
                                           const std::string& interface,
                                           const std::string& signalName,
                                           GVariant* /*parameters*/)
{
    LOGD("Received signal: " << senderBusName << "; " << objectPath << "; " << interface << "; "
         << signalName);
    if (objectPath == fake_power_manager_api::OBJECT_PATH &&
        interface == fake_power_manager_api::INTERFACE) {
        //power-manager sent us a signal, check it
        if (signalName == fake_power_manager_api::SIGNAL_DISPLAY_OFF && mSwitchToDefaultCallback) {
            mSwitchToDefaultCallback();
        }
    }
}

void ZoneConnection::sendNotification(const std::string& zone,
                                           const std::string& application,
                                           const std::string& message)
{
    GVariant* parameters = g_variant_new("(sss)",
                                         zone.c_str(),
                                         application.c_str(),
                                         message.c_str());
    mDbusConnection->emitSignal(api::zone::OBJECT_PATH,
                                api::zone::INTERFACE,
                                api::zone::SIGNAL_NOTIFICATION,
                                parameters);
}

void ZoneConnection::proxyCallAsync(const std::string& busName,
                                         const std::string& objectPath,
                                         const std::string& interface,
                                         const std::string& method,
                                         GVariant* parameters,
                                         const dbus::DbusConnection::AsyncMethodCallCallback& callback)
{
    mDbusConnection->callMethodAsync(busName,
                                     objectPath,
                                     interface,
                                     method,
                                     parameters,
                                     std::string(),
                                     callback);
}


} // namespace vasum
