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
 * @brief   Implementation of class for communication between container and server
 */

#include "container-connection.hpp"
#include "container-dbus-definitions.hpp"
#include "exception.hpp"

#include "utils/file-wait.hpp"
#include "utils/fs.hpp"
#include "utils/paths.hpp"
#include "log/logger.hpp"


namespace security_containers {

namespace {

// Timeout in ms for waiting for dbus transport.
// Should be very long to ensure dbus in container is ready.
const unsigned int TRANSPORT_READY_TIMEOUT = 2 * 60 * 1000;

// Timeout in ms for waiting for dbus name.
// Can happen if glib loop is busy or not present.
const unsigned int NAME_ACQUIRED_TIMEOUT = 5 * 1000;

} // namespace


ContainerConnection::ContainerConnection()
    : mNameAcquired(false)
    , mNameLost(false)
{
}


ContainerConnection::~ContainerConnection()
{
    deinitialize();
}


void ContainerConnection::initialize(const std::string& runMountPoint)
{
    if (runMountPoint.empty()) {
        return;
    }
    if (!utils::createDirectories(runMountPoint, 0755)) {
        LOGE("Initialization failed: could not create " << runMountPoint);
        throw ContainerConnectionException("Could not create: " + runMountPoint);
    }

    // try to umount if already mounted
    utils::umount(runMountPoint);

    if (!utils::mountTmpfs(runMountPoint)) {
        LOGE("Initialization failed: could not mount " << runMountPoint);
        throw ContainerConnectionException("Could not mount: " + runMountPoint);
    }

    mRunMountPoint = runMountPoint;
}

void ContainerConnection::deinitialize()
{
    if (!mRunMountPoint.empty()) {
        if (!utils::umount(mRunMountPoint)) {
            LOGE("Deinitialization failed: could not umount " << mRunMountPoint);
        }
        mRunMountPoint.clear();
    }
}


void ContainerConnection::connect()
{
    if (mRunMountPoint.empty()) {
        LOGW("The connection to the container is disabled");
        return;
    }

    const std::string dbusPath = mRunMountPoint + "/dbus/system_bus_socket";

    // TODO This should be done asynchronously.
    LOGT("Waiting for " << dbusPath);
    utils::waitForFile(dbusPath, TRANSPORT_READY_TIMEOUT);
    LOGT("Connecting to DBUS");
    mDbusConnection = dbus::DbusConnection::create("unix:path=" + dbusPath);
    LOGT("Setting DBUS name");

    mDbusConnection->setName(api::BUS_NAME,
                             std::bind(&ContainerConnection::onNameAcquired, this),
                             std::bind(&ContainerConnection::onNameLost, this));

    if (!waitForName(NAME_ACQUIRED_TIMEOUT)) {
        LOGE("Could not acquire dbus name: " << api::BUS_NAME);
        disconnect();
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


void ContainerConnection::disconnect()
{
    LOGD("Disconnecting");
    mDbusConnection.reset();

    std::unique_lock<std::mutex> lock(mNameMutex);
    mNameAcquired = false;
    mNameLost = false;
}

bool ContainerConnection::waitForName(const unsigned int timeoutMs)
{
    std::unique_lock<std::mutex> lock(mNameMutex);
    mNameCondition.wait_for(lock,
                            std::chrono::milliseconds(timeoutMs),
                            [this] {
                                return mNameAcquired || mNameLost;
                            });
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
    //TODO some callback?
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
