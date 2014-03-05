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
 * @file    dbus-client-test.cpp
 * @author  Piotr Bartosiewicz (p.bartosiewi@partner.samsung.com)
 * @brief   Example dbus api client
 */

#include "dbus-client-test.hpp"
#include "dbus-connection.hpp"
#include "dbus-test-common.hpp"


DbusClientTest::DbusClientTest()
{
    mConnection = DbusConnection::create(DBUS_ADDRESS);
}

void DbusClientTest::noop()
{
    GVariant* result = mConnection->callMethod(TESTAPI_BUS_NAME,
                                               TESTAPI_OBJECT_PATH,
                                               TESTAPI_INTERFACE,
                                               TESTAPI_METHOD_NOOP,
                                               NULL,
                                               NULL);
    g_variant_unref(result);
}

std::string DbusClientTest::process(const std::string& arg)
{
    GVariant* parameters = g_variant_new("(s)", arg.c_str());
    GVariant* result = mConnection->callMethod(TESTAPI_BUS_NAME,
                                               TESTAPI_OBJECT_PATH,
                                               TESTAPI_INTERFACE,
                                               TESTAPI_METHOD_PROCESS,
                                               parameters,
                                               G_VARIANT_TYPE("(s)"));
    const gchar* cresult;
    g_variant_get(result, "(&s)", &cresult);
    std::string ret = cresult;
    g_variant_unref(result);
    return ret;
}

void DbusClientTest::throwException(int arg)
{
    GVariant* parameters = g_variant_new("(i)", arg);
    GVariant* result = mConnection->callMethod(TESTAPI_BUS_NAME,
                                               TESTAPI_OBJECT_PATH,
                                               TESTAPI_INTERFACE,
                                               TESTAPI_METHOD_THROW,
                                               parameters,
                                               NULL);
    g_variant_unref(result);
}
