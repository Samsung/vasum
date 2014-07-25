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
 * @brief   Implementation of a class for communication with server
 */

#include "config.hpp"

#include "host-connection.hpp"
#include "host-dbus-definitions.hpp"
#include "exception.hpp"

#include "log/logger.hpp"


namespace security_containers {

namespace {

// Timeout in ms for waiting for dbus name.
// Can happen if glib loop is busy or not present.
// TODO: this should be in host's configuration file
const unsigned int NAME_ACQUIRED_TIMEOUT = 5 * 1000;

} // namespace


HostConnection::HostConnection()
    : mNameAcquired(false)
    , mNameLost(false)
{
    LOGT("Connecting to host system DBUS");
    mDbusConnection = dbus::DbusConnection::createSystem();

    LOGT("Setting DBUS name");
    mDbusConnection->setName(api::host::BUS_NAME,
                             std::bind(&HostConnection::onNameAcquired, this),
                             std::bind(&HostConnection::onNameLost, this));

    if (!waitForName(NAME_ACQUIRED_TIMEOUT)) {
        LOGE("Could not acquire dbus name: " << api::host::BUS_NAME);
        throw HostConnectionException("Could not acquire dbus name: " + api::host::BUS_NAME);
    }

    LOGT("Registering DBUS interface");
    using namespace std::placeholders;
    mDbusConnection->registerObject(api::host::OBJECT_PATH,
                                    api::host::DEFINITION,
                                    std::bind(&HostConnection::onMessageCall,
                                              this, _1, _2, _3, _4, _5));

    LOGD("Connected");
}

HostConnection::~HostConnection()
{
}

bool HostConnection::waitForName(const unsigned int timeoutMs)
{
    std::unique_lock<std::mutex> lock(mNameMutex);
    mNameCondition.wait_for(lock,
                            std::chrono::milliseconds(timeoutMs),
                            [this] {
                                return mNameAcquired || mNameLost;
                            });

    return mNameAcquired;
}

void HostConnection::onNameAcquired()
{
    std::unique_lock<std::mutex> lock(mNameMutex);
    mNameAcquired = true;
    mNameCondition.notify_one();
}

void HostConnection::onNameLost()
{
    std::unique_lock<std::mutex> lock(mNameMutex);
    mNameLost = true;
    mNameCondition.notify_one();

    if (mNameAcquired) {
        // TODO implement reconnecting
        LOGE("TODO Reconnect !!!");
    }
}

void HostConnection::setProxyCallCallback(const ProxyCallCallback& callback)
{
    mProxyCallCallback = callback;
}

void HostConnection::setGetContainerDbusesCallback(const GetContainerDbusesCallback& callback)
{
    mGetContainerDbusesCallback = callback;
}

void HostConnection::setGetContainerIdsCallback(const GetContainerIdsCallback& callback)
{
    mGetContainerIdsCallback = callback;
}

void HostConnection::setGetActiveContainerIdCallback(const GetActiveContainerIdCallback& callback)
{
    mGetActiveContainerIdCallback = callback;
}

void HostConnection::setSetActiveContainerCallback(const SetActiveContainerCallback& callback)
{
    mSetActiveContainerCallback = callback;
}

void HostConnection::onMessageCall(const std::string& objectPath,
                                        const std::string& interface,
                                        const std::string& methodName,
                                        GVariant* parameters,
                                        dbus::MethodResultBuilder::Pointer result)
{
    if (objectPath != api::host::OBJECT_PATH || interface != api::host::INTERFACE) {
        return;
    }

    if (methodName == api::host::METHOD_SET_ACTIVE_CONTAINER) {
        const gchar* id = NULL;
        g_variant_get(parameters, "(&s)", &id);

        if (mSetActiveContainerCallback) {
            mSetActiveContainerCallback(id, result);
        }
        return;
    }

    if (methodName == api::host::METHOD_GET_CONTAINER_DBUSES) {
        if (mGetContainerDbusesCallback) {
            mGetContainerDbusesCallback(result);
        }
        return;
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
        return;
    }

    if (methodName == api::host::METHOD_GET_CONTAINER_ID_LIST){
        if (mGetContainerIdsCallback){
            mGetContainerIdsCallback(result);
        }
        return;
    }

    if (methodName == api::host::METHOD_GET_ACTIVE_CONTAINER_ID){
        if (mGetActiveContainerIdCallback){
            mGetActiveContainerIdCallback(result);
        }
        return;
    }
}

void HostConnection::proxyCallAsync(const std::string& busName,
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

void HostConnection::signalContainerDbusState(const std::string& containerId,
                                              const std::string& dbusAddress)
{
    GVariant* parameters = g_variant_new("(ss)", containerId.c_str(), dbusAddress.c_str());
    mDbusConnection->emitSignal(api::host::OBJECT_PATH,
                                api::host::INTERFACE,
                                api::host::SIGNAL_CONTAINER_DBUS_STATE,
                                parameters);
}


} // namespace security_containers
