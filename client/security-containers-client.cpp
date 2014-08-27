/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Mateusz Malicki <m.malicki2@samsung.com>
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
 * @author  Mateusz Malicki (m.malicki2@samsung.com)
 * @brief   This file contains the public API for Security Containers Client
 */

#include <config.hpp>
#include "security-containers-client.h"
#include "security-containers-client-impl.hpp"

#include <cassert>

#ifndef API
#define API __attribute__((visibility("default")))
#endif // API

using namespace std;

namespace {

Client& getClient(ScClient client)
{
    assert(client);
    return *reinterpret_cast<Client*>(client);
}

} // namespace

/* external */
API ScStatus sc_start_glib_loop()
{
    return Client::sc_start_glib_loop();
}

API ScStatus sc_stop_glib_loop()
{
    return Client::sc_stop_glib_loop();
}

API ScClient sc_client_create()
{
    Client* clientPtr = new(nothrow) Client();
    return reinterpret_cast<ScClient>(clientPtr);
}

API ScStatus sc_connect(ScClient client)
{
    return getClient(client).createSystem();
}

API ScStatus sc_connect_custom(ScClient client, const char* address)
{
    return getClient(client).create(address);
}

API void sc_array_string_free(ScArrayString astring)
{
    if (!astring) {
        return;
    }
    for (char** ptr = astring; *ptr; ++ptr) {
        sc_string_free(*ptr);
    }
    free(astring);
}

API void sc_string_free(ScString string)
{
    free(string);
}


API void sc_client_free(ScClient client)
{
    if (client != NULL) {
        delete &getClient(client);
    }
}

API const char* sc_get_status_message(ScClient client)
{
    return getClient(client).sc_get_status_message();
}

API ScStatus sc_get_status(ScClient client)
{
    return getClient(client).sc_get_status();
}

API ScStatus sc_get_container_dbuses(ScClient client, ScArrayString* keys, ScArrayString* values)
{
    return getClient(client).sc_get_container_dbuses(keys, values);
}

API ScStatus sc_get_container_ids(ScClient client, ScArrayString* array)
{
    return getClient(client).sc_get_container_ids(array);
}

API ScStatus sc_get_active_container_id(ScClient client, ScString* id)
{
    return getClient(client).sc_get_active_container_id(id);
}

API ScStatus sc_set_active_container(ScClient client, const char* id)
{
    return getClient(client).sc_set_active_container(id);
}

API ScStatus sc_container_dbus_state(ScClient client,
                                     ScContainerDbusStateCallback containerDbusStateCallback,
                                     void* data)
{
    return getClient(client).sc_container_dbus_state(containerDbusStateCallback, data);
}

API ScStatus sc_notify_active_container(ScClient client,
                                        const char* application,
                                        const char* message)
{
    return getClient(client).sc_notify_active_container(application, message);
}

API ScStatus sc_file_move_request(ScClient client, const char* destContainer, const char* path)
{
    return getClient(client).sc_file_move_request(destContainer, path);
}

API ScStatus sc_notification(ScClient client,
                             ScNotificationCallback notificationCallback,
                             void* data)
{
    return getClient(client).sc_notification(notificationCallback, data);
}
