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

#include <cstdarg>
#include <cassert>

#ifndef API
#define API __attribute__((visibility("default")))
#endif // API

using namespace std;

namespace {

typedef Client* ClientPtr;

ClientPtr getClient(ScClient client)
{
    assert(client);
    return reinterpret_cast<ClientPtr>(client);
}

} // namespace

/* external */
API ScStatus sc_start()
{
    return Client::sc_start();
}

API ScStatus sc_stop()
{
    return Client::sc_stop();
}

API ScStatus sc_get_client(ScClient* client, ScClientType type, ...)
{
    const char* address = NULL;
    va_list vl;
    va_start(vl, type);
    if (type == SCCLIENT_CUSTOM_TYPE) {
        address = va_arg(vl, const char*);
        assert(address);
    }
    va_end(vl);

    assert(client);
    Client* clientPtr = new(nothrow) Client();
    *client = reinterpret_cast<ScClient>(clientPtr);
    if (clientPtr == NULL) {
        return SCCLIENT_NOT_ENOUGH_MEMORY;
    }

    ScStatus status;
    switch (type) {
    case SCCLIENT_CUSTOM_TYPE:
        status = clientPtr->create(address);
        break;
    case SCCLIENT_SYSTEM_TYPE:
        status = clientPtr->createSystem();
        break;
    default:
        assert(!"Logic error. No such ScClient type");
        status = SCCLIENT_EXCEPTION;
    }
    return status;
}

API int sc_is_failed(ScStatus status)
{
    return status != SCCLIENT_SUCCESS ? 1 : 0;
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
        delete getClient(client);
    }
}

API const char* sc_get_status_message(ScClient client)
{
    return getClient(client)->sc_get_status_message();
}

API ScStatus sc_get_status(ScClient client)
{
    return getClient(client)->sc_get_status();
}

API ScStatus sc_get_container_dbuses(ScClient client, ScArrayString* keys, ScArrayString* values)
{
    return getClient(client)->sc_get_container_dbuses(keys, values);
}
