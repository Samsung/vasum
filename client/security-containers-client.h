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
 *
 * @par Example usage:
 * @code
#include <stdio.h>
#include "client/security-containers-client.h"

int main(int argc, char** argv)
{
    ScStatus status;
    ScClient client;
    ScArrayString values = NULL;
    int ret = 0;

    status = sc_start_glib_loop(); // start glib loop (if not started any yet)
    if (SCCLIENT_SUCCESS != status) {
        // error!
        return 1;
    }

    client = sc_client_create(); // create client handle
    if (NULL == client) {
        // error!
        ret = 1;
        goto finish;
    }

    status = sc_connect(client); // connect to dbus
    if (SCCLIENT_SUCCESS != status) {
        // error!
        ret = 1;
        goto finish;
    }

    status = sc_get_container_ids(client, &values);
    if (SCCLIENT_SUCCESS != status) {
        // error!
        ret = 1;
        goto finish;
    }

    // print array
    for (ScArrayString iValues = values; *iValues; iValues++) {
        printf("%s\n", *iValues);
    }

finish:
    sc_array_string_free(values); // free memory
    sc_client_free(client); // destroy client handle
    sc_stop_glib_loop(); // stop the glib loop (use only with sc_start_glib_loop)
    return ret;
}
 @endcode
 */

#ifndef SECURITY_CONTAINERS_CLIENT_H
#define SECURITY_CONTAINERS_CLIENT_H

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * security-containers-server's client pointer.
 */
typedef void* ScClient;

/**
 * NULL-terminated string type.
 *
 * @sa sc_array_string_free
 */
typedef char* ScString;

/**
 * NULL-terminated array of strings type.
 *
 * @sa sc_string_free
 */
typedef ScString* ScArrayString;

/**
 * Completion status of communication function.
 */
typedef enum {
    SCCLIENT_CUSTOM_ERROR,     ///< User specified error
    SCCLIENT_IO_ERROR,         ///< Input/Output error
    SCCLIENT_OPERATION_FAILED, ///< Operation failed
    SCCLIENT_INVALID_ARGUMENT, ///< Invalid argument
    SCCLIENT_OTHER_ERROR,      ///< Other error
    SCCLIENT_SUCCESS           ///< Success
} ScStatus;

/**
 * Start glib loop.
 *
 * Do not call this function if an application creates a glib loop itself.
 * Otherwise, call it before any other function from this library.
 *
 * @return status of this function call
 */
ScStatus sc_start_glib_loop();

/**
 * Stop glib loop.
 *
 * Call only if sc_start_glib_loop() was called.
 *
 * @return status of this function call
 */
ScStatus sc_stop_glib_loop();

/**
 * Create a new security-containers-server's client.
 *
 * @return Created client pointer or NULL on failure.
 */
ScClient sc_client_create();

/**
 * Release client resources.
 *
 * @param[in] client security-containers-server's client
 */
void sc_client_free(ScClient client);

/**
 * Get status code of last security-containers-server communication.
 *
 * @param[in] client security-containers-server's client
 * @return status of this function call
 */
ScStatus sc_get_status(ScClient client);

/**
 * Get status message of the last security-containers-server communication.
 *
 * @param[in] client security-containers-server's client
 * @return last status message from security-containers-server communication
 */
const char* sc_get_status_message(ScClient client);

/**
 * Connect client to the security-containers-server.
 *
 * @param[in] client security-containers-server's client
 * @return status of this function call
 */
ScStatus sc_connect(ScClient client);

/**
 * Connect client to the security-containers-server via custom address.
 *
 * @param[in] client security-containers-server's client
 * @param[in] address dbus address
 * @return status of this function call
 */
ScStatus sc_connect_custom(ScClient client, const char* address);

/**
 * Release ScArrayString.
 *
 * @param[in] astring ScArrayString
 */
void sc_array_string_free(ScArrayString astring);

/**
 * Release ScString.
 *
 * @param string ScString
 */
void sc_string_free(ScString string);


/**
 * @name Host API
 *
 * Functions using org.tizen.containers.host.manager D-Bus interface.
 *
 * @{
 */

/**
 * Container's D-Bus state change callback function signature.
 *
 * @param[in] containerId affected container id
 * @param[in] dbusAddress new D-Bus address
 * @param data custom user's data pointer passed to sc_container_dbus_state() function
 */
typedef void (*ScContainerDbusStateCallback)(const char* containerId,
                                             const char* dbusAddress,
                                             void* data);

/**
 * Get dbus address of each container.
 *
 * @param[in] client security-containers-server's client
 * @param[out] keys array of containers name
 * @param[out] values array of containers dbus address
 * @return status of this function call
 * @post keys[i] corresponds to values[i]
 * @remark Use sc_array_string_free() to free memory occupied by @p keys and @p values.
 */
ScStatus sc_get_container_dbuses(ScClient client, ScArrayString* keys, ScArrayString* values);

/**
 * Get containers name.
 *
 * @param[in] client security-containers-server's client
 * @param[out] array array of containers name
 * @return status of this function call
 * @remark Use sc_array_string_free() to free memory occupied by @p array.
 */
ScStatus sc_get_container_ids(ScClient client, ScArrayString* array);

/**
 * Get active (foreground) container name.
 *
 * @param[in] client security-containers-server's client
 * @param[out] id active container name
 * @return status of this function call
 * @remark Use @p sc_string_free() to free memory occupied by @p id.
 */
ScStatus sc_get_active_container_id(ScClient client, ScString* id);

/**
 * Set active (foreground) container.
 *
 * @param[in] client security-containers-server's client
 * @param[in] id container name
 * @return status of this function call
 */
ScStatus sc_set_active_container(ScClient client, const char* id);

/**
 * Register dbus state change callback function.
 *
 * @note The callback function will be invoked on a different thread.
 *
 * @param[in] client security-containers-server's client
 * @param[in] containerDbusStateCallback callback function
 * @param[in] data some extra data that will be passed to callback function
 * @return status of this function call
 */
ScStatus sc_container_dbus_state(ScClient client,
                                 ScContainerDbusStateCallback containerDbusStateCallback,
                                 void* data);

/** @} */ // Host API


/**
 * @name Domain API
 *
 * Functions using org.tizen.containers.domain.manager D-Bus interface.
 *
 * @{
 */

/**
 * Notification callback function signature.
 *
 * @param[in] container source container
 * @param[in] application sending application name
 * @param[in] message notification message
 * @param data custom user's data pointer passed to sc_notification()
 */
typedef void (*ScNotificationCallback)(const char* container,
                                       const char* application,
                                       const char* message,
                                       void* data);
/**
 * Send message to active container.
 *
 * @param[in] client security-containers-server's client
 * @param[in] application application name
 * @param[in] message message
 * @return status of this function call
 */
ScStatus sc_notify_active_container(ScClient client, const char* application, const char* message);

/**
 * Move file between containers.
 *
 * @param[in] client security-containers-server's client
 * @param[in] destContainer destination container id
 * @param[in] path path to moved file
 * @return status of this function call
 */
ScStatus sc_file_move_request(ScClient client, const char* destContainer, const char* path);

/**
 * Register notification callback function.
 *
 * @note The callback function will be invoked on a different thread.
 *
 * @param[in] client security-containers-server's client
 * @param[in] notificationCallback callback function
 * @param[in] data some extra data that will be passed to callback function
 * @return status of this function call
 */
ScStatus sc_notification(ScClient client, ScNotificationCallback notificationCallback, void* data);

/** @} */ // Domain API

#ifdef __cplusplus
}
#endif

#endif /* SECRITY_CONTAINERS_CLIENT_H */
