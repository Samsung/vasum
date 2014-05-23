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
 * @brief   Implementation of a class for communication between container and server
 */

#include "config.hpp"

#include "container-connection.hpp"
#include "container-dbus-definitions.hpp"
#include "exception.hpp"

#include "log/logger.hpp"


namespace security_containers {

namespace {

// Timeout in ms for waiting for dbus name.
// Can happen if glib loop is busy or not present.
// TODO: this should be in container's configuration file
const unsigned int NAME_ACQUIRED_TIMEOUT = 5 * 1000;

} // namespace


ContainerConnection::ContainerConnection(const std::string& address, const OnNameLostCallback& callback)
    : mNameAcquired(false)
    , mNameLost(false)
{
    if (address.empty()) {
        // TODO: this should throw. Don't return cleanly unless the object is fully usable.
        LOGW("The connection to the container is disabled");
        return;
    }

    LOGT("Connecting to DBUS on " << address);
    mDbusConnection = dbus::DbusConnection::create(address);

    LOGT("Setting DBUS name");
    mDbusConnection->setName(api::BUS_NAME,
                             std::bind(&ContainerConnection::onNameAcquired, this),
                             std::bind(&ContainerConnection::onNameLost, this));

    if (!waitForNameAndSetCallback(NAME_ACQUIRED_TIMEOUT, callback)) {
        LOGE("Could not acquire dbus name: " << api::BUS_NAME);
        throw ContainerConnectionException("Could not acquire dbus name: " + api::BUS_NAME);
    }

    LOGT("Registering DBUS interface");
    using namespace std::placeholders;
    mDbusConnection->registerObject(api::OBJECT_PATH,
                                    api::DEFINITION,
                                    std::bind(&ContainerConnection::onMessageCall,
                                              this,
                                              _1,
                                              _2,
                                              _3,
                                              _4,
                                              _5));
    LOGD("Connected");
}

ContainerConnection::~ContainerConnection()
{
}

bool ContainerConnection::waitForNameAndSetCallback(const unsigned int timeoutMs, const OnNameLostCallback& callback)
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

void ContainerConnection::onNameAcquired()
{
    std::unique_lock<std::mutex> lock(mNameMutex);
    mNameAcquired = true;
    mNameCondition.notify_one();
}

void ContainerConnection::onNameLost()
{
    std::unique_lock<std::mutex> lock(mNameMutex);
    mNameLost = true;
    mNameCondition.notify_one();

    if (mOnNameLostCallback) {
        mOnNameLostCallback();
    }
}

void ContainerConnection::setNotifyActiveContainerCallback(
    const NotifyActiveContainerCallback& callback)
{
    mNotifyActiveContainerCallback = callback;
}

void ContainerConnection::onMessageCall(const std::string& objectPath,
                                        const std::string& interface,
                                        const std::string& methodName,
                                        GVariant* parameters,
                                        dbus::MethodResultBuilder& result)
{
    if (objectPath != api::OBJECT_PATH || interface != api::INTERFACE) {
        return;
    }

    if (methodName == api::METHOD_NOTIFY_ACTIVE_CONTAINER) {
        const gchar* application = NULL;
        const gchar* message = NULL;
        g_variant_get(parameters, "(&s&s)", &application, &message);
        if (mNotifyActiveContainerCallback) {
            mNotifyActiveContainerCallback(application, message);
            result.setVoid();
        }
    }
}

void ContainerConnection::sendNotification(const std::string& container,
                                           const std::string& application,
                                           const std::string& message)
{
    GVariant* parameters = g_variant_new("(sss)",
                                         container.c_str(),
                                         application.c_str(),
                                         message.c_str());
    mDbusConnection->emitSignal(api::OBJECT_PATH,
                                api::INTERFACE,
                                api::SIGNAL_NOTIFICATION,
                                parameters);
}


} // namespace security_containers
