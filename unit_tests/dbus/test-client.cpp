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
 * @file
 * @author  Piotr Bartosiewicz (p.bartosiewi@partner.samsung.com)
 * @brief   Example dbus api client
 */

#include "dbus/test-client.hpp"
#include "dbus/test-common.hpp"

#include "dbus/connection.hpp"


namespace security_containers {


DbusTestClient::DbusTestClient()
{
    mConnection = dbus::DbusConnection::create(DBUS_ADDRESS);
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


} // namespace security_containers
