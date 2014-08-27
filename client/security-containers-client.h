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
 * Do not call this function if the application creates glib loop itself.
 * Otherwise call it before any other function from this library.
 *
 * @return status of this function call
 */
ScStatus sc_start_glib_loop();

/**
 * Stop glib loop.
 *
 * @return status of this function call
 */
ScStatus sc_stop_glib_loop();

/**
 * Create a new security-containers-server's client.
 *
 * @return created client
 */
ScClient sc_client_create();

/**
 * Release client resources.
 *
 * @param client security-containers-server's client
 */
void sc_client_free(ScClient client);

/**
 * Get status code of last security-containers-server communication.
 *
 * @param client security-containers-server's client
 * @return status of this function call
 */
ScStatus sc_get_status(ScClient client);

/**
 * Get status message of the last security-containers-server communication.
 *
 * @param client security-containers-server's client
 * @return last status message from security-containers-server communication
 */
const char* sc_get_status_message(ScClient client);

/**
 * Connect client to the security-containers-server.
 *
 * @param client security-containers-server's client
 * @return status of this function call
 */
ScStatus sc_connect(ScClient client);

/**
 * Connect client to the security-containers-server via custom address.
 *
 * @param client security-containers-server's client
 * @param address dbus address
 * @return status of this function call
 */
ScStatus sc_connect_custom(ScClient client, const char* address);

/**
 * Release ScArrayString.
 *
 * @param astring ScArrayString
 */
void sc_array_string_free(ScArrayString astring);

/**
 * Release ScString.
 *
 * @param string ScString
 */
void sc_string_free(ScString string);


/*************************************************************************************************
 *
 *  org.tizen.containers.host.manager interface
 *
 ************************************************************************************************/

/**
 * Dbus state change callback function signature.
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
 */
ScStatus sc_get_container_dbuses(ScClient client, ScArrayString* keys, ScArrayString* values);

/**
 * Get containers name.
 *
 * @param[in] client security-containers-server's client
 * @param[out] array array of containers name
 * @return status of this function call
 */
ScStatus sc_get_container_ids(ScClient client, ScArrayString* array);

/**
 * Get active container name.
 *
 * @param[in] client security-containers-server's client
 * @param[out] id active container name
 * @return status of this function call
 */
ScStatus sc_get_active_container_id(ScClient client, ScString* id);

/**
 * Set active container.
 *
 * @param client security-containers-server's client
 * @param id container name
 * @return status of this function call
 */
ScStatus sc_set_active_container(ScClient client, const char* id);

/**
 * Register dbus state change callback function.
 *
 * The callback function will be invoked on a different thread
 *
 * @param client security-containers-server's client
 * @param containerDbusStateCallback callback function
 * @param data some extra data that will be passed to callback function
 * @return status of this function call
 */
ScStatus sc_container_dbus_state(ScClient client,
                                 ScContainerDbusStateCallback containerDbusStateCallback,
                                 void* data);


/*************************************************************************************************
 *
 *  org.tizen.containers.domain.manager interface
 *
 ************************************************************************************************/

/**
 * Notification callback function signature.
 */
typedef void (*ScNotificationCallback)(const char* container,
                                       const char* application,
                                       const char* message,
                                       void* data);
/**
 * Send message to active container.
 *
 * @param client security-containers-server's client
 * @param application application name
 * @param message message
 * @return status of this function call
 */
ScStatus sc_notify_active_container(ScClient client, const char* application, const char* message);

/**
 * Register notification callback function.
 *
 * The callback function will be invoked on a different thread.
 *
 * @param client security-containers-server's client
 * @param notificationCallback callback function
 * @param data some extra data that will be passed to callback function
 * @return status of this function call
 */
ScStatus sc_notification(ScClient client, ScNotificationCallback notificationCallback, void* data);
#ifdef __cplusplus
}
#endif

#endif /* SECRITY_CONTAINERS_CLIENT_H */
