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
 * @brief   Example dbus api client
 */

#ifdef DBUS_CONNECTION
#include "config.hpp"

#include "dbus/test-client.hpp"
#include "dbus/test-common.hpp"

#include "dbus/connection.hpp"


namespace vasum {


DbusTestClient::DbusTestClient()
    : mConnection(dbus::DbusConnection::create(DBUS_ADDRESS))
{
    using namespace std::placeholders;
    mConnection->signalSubscribe(std::bind(&DbusTestClient::onSignal, this, _1, _2, _3, _4, _5),
                                 TESTAPI_BUS_NAME);
}

void DbusTestClient::setNotifyCallback(const NotifyCallback& callback)
{
    mNotifyCallback = callback;
}

void DbusTestClient::onSignal(const std::string& /*senderBusName*/,
                              const std::string& objectPath,
                              const std::string& interface,
                              const std::string& signalName,
                              GVariant* parameters)
{
    if (objectPath != TESTAPI_OBJECT_PATH || interface != TESTAPI_INTERFACE) {
        return;
    }
    if (signalName == TESTAPI_SIGNAL_NOTIFY) {
        if (mNotifyCallback) {
            const gchar* message;
            g_variant_get(parameters, "(&s)", &message);
            mNotifyCallback(message);
        }
    }
}

void DbusTestClient::noop()
{
    mConnection->callMethod(TESTAPI_BUS_NAME,
                            TESTAPI_OBJECT_PATH,
                            TESTAPI_INTERFACE,
                            TESTAPI_METHOD_NOOP,
                            NULL,
                            "()");
}

std::string DbusTestClient::process(const std::string& arg)
{
    GVariant* parameters = g_variant_new("(s)", arg.c_str());
    dbus::GVariantPtr result = mConnection->callMethod(TESTAPI_BUS_NAME,
                                                       TESTAPI_OBJECT_PATH,
                                                       TESTAPI_INTERFACE,
                                                       TESTAPI_METHOD_PROCESS,
                                                       parameters,
                                                       "(s)");
    const gchar* cresult;
    g_variant_get(result.get(), "(&s)", &cresult);
    return cresult;
}

void DbusTestClient::throwException(int arg)
{
    GVariant* parameters = g_variant_new("(i)", arg);
    mConnection->callMethod(TESTAPI_BUS_NAME,
                            TESTAPI_OBJECT_PATH,
                            TESTAPI_INTERFACE,
                            TESTAPI_METHOD_THROW,
                            parameters,
                            "()");
}


} // namespace vasum
#endif //DBUS_CONNECTION
