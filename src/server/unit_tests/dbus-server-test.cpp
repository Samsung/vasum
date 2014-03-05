/*
 *  Copyright (c) 2000 - 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Bumjin Im <bj.im@samsung.com>
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
 * @file    dbus-server-test.cpp
 * @author  Piotr Bartosiewicz (p.bartosiewi@partner.samsung.com)
 * @brief   Example dbus api server
 */

#include "dbus-server-test.hpp"
#include "dbus-connection.hpp"
#include "dbus-exception.hpp"
#include "dbus-test-common.hpp"
#include "scs-log.hpp"


DbusServerTest::DbusServerTest()
    : mNameAcquired(false)
    , mPendingDisconnect(false)
{
    mConnection = DbusConnection::create(DBUS_ADDRESS);
    mConnection->setName(TESTAPI_BUS_NAME,
                         std::bind(&DbusServerTest::onNameAcquired, this),
                         std::bind(&DbusServerTest::onDisconnect, this));
    if (!waitForName()) {
        mConnection.reset();
        throw DbusConnectException("Could not acquire name");
    }
    using namespace std::placeholders;
    mConnection->registerObject(TESTAPI_OBJECT_PATH, TESTAPI_DEFINITION,
                                std::bind(&DbusServerTest::onMessageCall, this, _1, _2, _3, _4, _5));
}

bool DbusServerTest::waitForName()
{
    std::unique_lock<std::mutex> lock(mMutex);
    mNameCondition.wait(lock, [this] {return mNameAcquired || mPendingDisconnect;});
    return mNameAcquired;
}

void DbusServerTest::setDisconnectCallback(const DisconnectCallback& callback)
{
    std::unique_lock<std::mutex> lock(mMutex);
    mDisconnectCallback = callback;
    if (mPendingDisconnect) {
        mPendingDisconnect = false;
        mDisconnectCallback();
    }

}

void DbusServerTest::onNameAcquired()
{
    std::unique_lock<std::mutex> lock(mMutex);
    mNameAcquired = true;
    mNameCondition.notify_one();
}

void DbusServerTest::onDisconnect()
{
    std::unique_lock<std::mutex> lock(mMutex);
    if (mDisconnectCallback) {
        mDisconnectCallback();
    } else {
        mPendingDisconnect = true;
        mNameCondition.notify_one();
    }
}

void DbusServerTest::noop()
{
}

std::string DbusServerTest::process(const std::string& arg)
{
    return "Processed: " + arg;
}

void DbusServerTest::throwException(int arg)
{
    if (arg != 0) {
        throw std::runtime_error("Argument: " + std::to_string(arg));
    }
}

void DbusServerTest::onMessageCall(
    const std::string& objectPath,
    const std::string& interface,
    const std::string& method,
    GVariant* parameters,
    MethodResultBuilder& result)
{
    try {
        if (objectPath != TESTAPI_OBJECT_PATH || interface != TESTAPI_INTERFACE) {
            throw std::logic_error("unsupported interface");
        }

        if (method == TESTAPI_METHOD_NOOP) {
            noop();
            result.setVoid();
        } else if (method == TESTAPI_METHOD_PROCESS) {
            const gchar* arg;
            g_variant_get(parameters, "(&s)", &arg);
            std::string ret = process(arg);
            GVariant* variant = g_variant_new("(s)", ret.c_str());
            result.set(variant);
        } else if (method == TESTAPI_METHOD_THROW) {
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
