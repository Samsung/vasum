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
 * @brief   Example dbus api server
 */

#include "config.hpp"

#include "dbus/test-server.hpp"
#include "dbus/test-common.hpp"

#include "dbus/connection.hpp"
#include "dbus/exception.hpp"
#include "log/logger.hpp"


namespace security_containers {


DbusTestServer::DbusTestServer()
    : mConnection(dbus::DbusConnection::create(DBUS_ADDRESS))
    , mNameAcquired(false)
    , mPendingDisconnect(false)
{
    mConnection->setName(TESTAPI_BUS_NAME,
                         std::bind(&DbusTestServer::onNameAcquired, this),
                         std::bind(&DbusTestServer::onDisconnect, this));
    if (!waitForName()) {
        mConnection.reset();
        throw dbus::DbusOperationException("Could not acquire name");
    }
    using namespace std::placeholders;
    mConnection->registerObject(TESTAPI_OBJECT_PATH,
                                TESTAPI_DEFINITION,
                                std::bind(&DbusTestServer::onMessageCall, this, _1, _2, _3, _4, _5));
}

bool DbusTestServer::waitForName()
{
    std::unique_lock<std::mutex> lock(mMutex);
    mNameCondition.wait(lock, [this] {return mNameAcquired || mPendingDisconnect;});
    return mNameAcquired;
}

void DbusTestServer::setDisconnectCallback(const DisconnectCallback& callback)
{
    std::unique_lock<std::mutex> lock(mMutex);
    mDisconnectCallback = callback;
    if (mPendingDisconnect) {
        mPendingDisconnect = false;
        mDisconnectCallback();
    }

}

void DbusTestServer::onNameAcquired()
{
    std::unique_lock<std::mutex> lock(mMutex);
    mNameAcquired = true;
    mNameCondition.notify_one();
}

void DbusTestServer::onDisconnect()
{
    std::unique_lock<std::mutex> lock(mMutex);
    if (mDisconnectCallback) {
        mDisconnectCallback();
    } else {
        mPendingDisconnect = true;
        mNameCondition.notify_one();
    }
}

void DbusTestServer::noop()
{
}

std::string DbusTestServer::process(const std::string& arg)
{
    return "Processed: " + arg;
}

void DbusTestServer::throwException(int arg)
{
    if (arg != 0) {
        throw std::runtime_error("Argument: " + std::to_string(arg));
    }
}

void DbusTestServer::notifyClients(const std::string& message)
{
    mConnection->emitSignal(TESTAPI_OBJECT_PATH,
                            TESTAPI_INTERFACE,
                            TESTAPI_SIGNAL_NOTIFY,
                            g_variant_new("(s)", message.c_str()));
}

void DbusTestServer::onMessageCall(const std::string& objectPath,
                                   const std::string& interface,
                                   const std::string& methodName,
                                   GVariant* parameters,
                                   dbus::MethodResultBuilder& result)
{
    try {
        if (objectPath != TESTAPI_OBJECT_PATH || interface != TESTAPI_INTERFACE) {
            throw std::logic_error("unsupported interface");
        }

        if (methodName == TESTAPI_METHOD_NOOP) {
            noop();
            result.setVoid();
        } else if (methodName == TESTAPI_METHOD_PROCESS) {
            const gchar* arg;
            g_variant_get(parameters, "(&s)", &arg);
            std::string ret = process(arg);
            GVariant* variant = g_variant_new("(s)", ret.c_str());
            result.set(variant);
        } else if (methodName == TESTAPI_METHOD_THROW) {
            int arg;
            g_variant_get(parameters, "(i)", &arg);
            throwException(arg);
            result.setVoid();
        } else {
            LOGE("unknown method; should never happen");
        }
    } catch (const std::exception& e) {
        result.setError("com.samsung.Exception", e.what());
    }
}


} // namespace security_containers
