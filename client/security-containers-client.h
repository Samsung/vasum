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

#ifndef SECURITY_CONTAINERS_CLIENT_H
#define SECURITY_CONTAINERS_CLIENT_H

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * security-containers-server's client
 */
typedef void* ScClient;

/**
 * String type.
 *
 * NULL-terminated string.
 */
typedef char* ScString;

/**
 * Array of strings type.
 *
 * Array are NULL-terminated.
 */
typedef ScString* ScArrayString;

/**
 * Completion status of communication function.
 */
typedef enum {
    SCCLIENT_DBUS_CUSTOM_EXCEPTION,
    SCCLIENT_DBUS_IO_EXCEPTION,
    SCCLIENT_DBUS_OPERATION_EXCEPTION,
    SCCLIENT_DBUS_INVALID_ARGUMENT_EXCEPTION,
    SCCLIENT_DBUS_EXCEPTION,
    SCCLIENT_NOT_ENOUGH_MEMORY,
    SCCLIENT_RUNTIME_EXCEPTION,
    SCCLIENT_EXCEPTION,
    SCCLIENT_SUCCESS
} ScStatus;

typedef enum {
    SCCLIENT_SYSTEM_TYPE,
    SCCLIENT_CUSTOM_TYPE
} ScClientType;

/**
 * Initialize communication resources.
 */
ScStatus sc_start();

/**
 * Release communication resources.
 *
 * @return Status of this function call.
 */
ScStatus sc_stop();

/**
 * Create a security-containers-server's client.
 *
 * After calling this function a connection to security-containers-server is established.
 *
 * @param[out] client security-containers-server's client who will be returned.
 *                    Client can be broken. To check this you must call sc_is_failed().
 *                    Broken client can't be used to communicate with security-containers-server.
 * @param[in] type Type of client.
 * @param[in] @optional address Dbus socket address (significant only for type SCCLIENT_CUSTOM_TYPE).
 * @return Status of this function call.
 */
ScStatus sc_get_client(ScClient* client, ScClientType type, /* const char* address */ ...);

/**
 * Release client resources.
 *
 * @param client security-containers-server's client.
 */
void sc_client_free(ScClient client);

/**
 * Get status message of the last security-containers-server communication.
 *
 * @param client security-containers-server's client.
 * @return Last status message from security-containers-server communication.
 */
const char* sc_get_status_message(ScClient client);

/**
 * Get status code of last security-containers-server communication.
 *
 * @param client security-containers-server's client.
 * @return Status of this function call.
 */
ScStatus sc_get_status(ScClient client);

/**
 * Check if security-containers-server communication function fail.
 *
 * @param status Value returned by security-containers-server communication function.
 * @return 0 if succeeded otherwise 1.
 */
int sc_is_failed(ScStatus status);

/**
 * Release ScArrayString.
 *
 * @param astring ScArrayString.
 */
void sc_array_string_free(ScArrayString astring);

/**
 * Release ScString.
 *
 * @param string ScString.
 */
void sc_string_free(ScString string);


/*************************************************************************************************
 *
 *  org.tizen.containers.host.manager interface
 *
 ************************************************************************************************/

/**
 * Get dbus address of each container.
 *
 * @param[in] client security-containers-server's client.
 * @param[out] keys Array of containers name.
 * @param[out] values Array of containers dbus address.
 * @return Status of this function call.
 * @post keys[i] corresponds to values[i].
 */
ScStatus sc_get_container_dbuses(ScClient client, ScArrayString* keys, ScArrayString* values);

#ifdef __cplusplus
}
#endif

#endif /* SECRITY_CONTAINERS_CLIENT_H */
