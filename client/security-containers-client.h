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
    VsmStatus status;
    VsmClient client;
    VsmArrayString values = NULL;
    int ret = 0;

    status = vsm_start_glib_loop(); // start glib loop (if not started any yet)
    if (VSMCLIENT_SUCCESS != status) {
        // error!
        return 1;
    }

    client = vsm_client_create(); // create client handle
    if (NULL == client) {
        // error!
        ret = 1;
        goto finish;
    }

    status = vsm_connect(client); // connect to dbus
    if (VSMCLIENT_SUCCESS != status) {
        // error!
        ret = 1;
        goto finish;
    }

    status = vsm_get_domain_ids(client, &values);
    if (VSMCLIENT_SUCCESS != status) {
        // error!
        ret = 1;
        goto finish;
    }

    // print array
    for (VsmArrayString iValues = values; *iValues; iValues++) {
        printf("%s\n", *iValues);
    }

finish:
    vsm_array_string_free(values); // free memory
    vsm_client_free(client); // destroy client handle
    vsm_stop_glib_loop(); // stop the glib loop (use only with vsm_start_glib_loop)
    return ret;
}
 @endcode
 */

#ifndef SECURITY_CONTAINERS_CLIENT_H
#define SECURITY_CONTAINERS_CLIENT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * security-containers-server's client pointer.
 */
typedef void* VsmClient;

/**
 * NULL-terminated string type.
 *
 * @sa vsm_array_string_free
 */
typedef char* VsmString;

/**
 * NULL-terminated array of strings type.
 *
 * @sa vsm_string_free
 */
typedef VsmString* VsmArrayString;

/**
 * Completion status of communication function.
 */
typedef enum {
    VSMCLIENT_CUSTOM_ERROR,     ///< User specified error
    VSMCLIENT_IO_ERROR,         ///< Input/Output error
    VSMCLIENT_OPERATION_FAILED, ///< Operation failed
    VSMCLIENT_INVALID_ARGUMENT, ///< Invalid argument
    VSMCLIENT_OTHER_ERROR,      ///< Other error
    VSMCLIENT_SUCCESS           ///< Success
} VsmStatus;

/**
 * Subscription id
 */
typedef unsigned int VsmSubscriptionId;

/**
 * States of domain
 */
typedef enum {
    STOPPED,
    STARTING,
    RUNNING,
    STOPPING,
    ABORTING,
    FREEZING,
    FROZEN,
    THAWED,
    LOCKED,
    MAX_STATE,
    ACTIVATING = 128
} VsmDomainState;

/**
 * Domain information structure
 */
typedef struct {
    char* id;
    int terminal;
    VsmDomainState state;
    char *rootfs_path;
} VsmDomainStructure;

/**
 * Domain information
 */
typedef VsmDomainStructure* VsmDomain;

/**
 * Netowrk device type
 */
typedef enum {
    VETH,
    PHYS,
    MACVLAN
} VsmNetdevType;

/**
 * Network device information structure
 */
typedef struct {
    char* name;
    VsmNetdevType type;
} VsmNetdevStructure;

/**
 * Network device information
 */
typedef VsmNetdevStructure* VsmNetdev;

/**
 * Start glib loop.
 *
 * Do not call this function if an application creates a glib loop itself.
 * Otherwise, call it before any other function from this library.
 *
 * @return status of this function call
 */
VsmStatus vsm_start_glib_loop();

/**
 * Stop glib loop.
 *
 * Call only if vsm_start_glib_loop() was called.
 *
 * @return status of this function call
 */
VsmStatus vsm_stop_glib_loop();

/**
 * Create a new security-containers-server's client.
 *
 * @return Created client pointer or NULL on failure.
 */
VsmClient vsm_client_create();

/**
 * Release client resources.
 *
 * @param[in] client security-containers-server's client
 */
void vsm_client_free(VsmClient client);

/**
 * Get status code of last security-containers-server communication.
 *
 * @param[in] client security-containers-server's client
 * @return status of this function call
 */
VsmStatus vsm_get_status(VsmClient client);

/**
 * Get status message of the last security-containers-server communication.
 *
 * @param[in] client security-containers-server's client
 * @return last status message from security-containers-server communication
 */
const char* vsm_get_status_message(VsmClient client);

/**
 * Connect client to the security-containers-server.
 *
 * @param[in] client security-containers-server's client
 * @return status of this function call
 */
VsmStatus vsm_connect(VsmClient client);

/**
 * Connect client to the security-containers-server via custom address.
 *
 * @param[in] client security-containers-server's client
 * @param[in] address dbus address
 * @return status of this function call
 */
VsmStatus vsm_connect_custom(VsmClient client, const char* address);

/**
 * Release VsmArrayString.
 *
 * @param[in] astring VsmArrayString
 */
void vsm_array_string_free(VsmArrayString astring);

/**
 * Release VsmString.
 *
 * @param string VsmString
 */
void vsm_string_free(VsmString string);

/**
 * Release VsmDomain
 *
 * @param domain VsmDomain
 */
void vsm_domain_free(VsmDomain domain);

/**
 * Release VsmNetdev
 *
 * @param netdev VsmNetdev
 */
void vsm_netdev_free(VsmNetdev netdev);

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
 * @param data custom user's data pointer passed to vsm_add_state_callback() function
 */
typedef void (*VsmContainerDbusStateCallback)(const char* containerId,
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
 * @remark Use vsm_array_string_free() to free memory occupied by @p keys and @p values.
 */
VsmStatus vsm_get_container_dbuses(VsmClient client, VsmArrayString* keys, VsmArrayString* values);

/**
 * Get containers name.
 *
 * @param[in] client security-containers-server's client
 * @param[out] array array of containers name
 * @return status of this function call
 * @remark Use vsm_array_string_free() to free memory occupied by @p array.
 */
VsmStatus vsm_get_domain_ids(VsmClient client, VsmArrayString* array);

/**
 * Get active (foreground) container name.
 *
 * @param[in] client security-containers-server's client
 * @param[out] id active container name
 * @return status of this function call
 * @remark Use @p vsm_string_free() to free memory occupied by @p id.
 */
VsmStatus vsm_get_active_container_id(VsmClient client, VsmString* id);

/**
 * Get container name of process with given pid.
 *
 * @param[in] client security-containers-server's client
 * @param[in] pid process id
 * @param[out] id active container name
 * @return status of this function call
 * @remark Use @p vsm_string_free() to free memory occupied by @p id.
 */
VsmStatus vsm_lookup_domain_by_pid(VsmClient client, int pid, VsmString* id);

/**
 * Get domain informations of domain with given id.
 *
 * @param[in] client security-containers-server's client
 * @param[in] id domain name
 * @param[out] domain domain informations
 * @return status of this function call
 * @remark Use @p vsm_domain_free() to free memory occupied by @p domain
 */
VsmStatus vsm_lookup_domain_by_id(VsmClient client, const char* id, VsmDomain* domain);

/**
 * Get domain name with given terminal.
 *
 * @param[in] client security-containers-server's client
 * @param[in] terminal terminal id
 * @param[out] id domain name with given terminal
 * @return status of this function call
 * @remark Use @p vsm_string_free() to free memory occupied by @p id.
 */
VsmStatus vsm_lookup_domain_by_terminal_id(VsmClient client, int terminal, VsmString* id);

/**
 * Set active (foreground) container.
 *
 * @param[in] client security-containers-server's client
 * @param[in] id container name
 * @return status of this function call
 */
VsmStatus vsm_set_active_container(VsmClient client, const char* id);

/**
 * Create and add container
 *
 * @param[in] client security-containers-server's client
 * @param[in] id container id
 * @param[in] tname template name, NULL for default
 * @return status of this function call
 */
VsmStatus vsm_create_domain(VsmClient client, const char* id, const char* tname);

/**
 * Remove domain
 *
 * @param[in] client security-containers-server's client
 * @param[in] id container id
 * @param[in] force if 0 data will be kept, otherwise data will be lost
 * @return status of this function call
 */
VsmStatus vsm_destroy_domain(VsmStatus clent, const char* id, int force);

/**
 * Shutdown domain
 *
 * @param[in] client security-containers-server's client
 * @param[in] id domain name
 * @return status of this function call
 */
VsmStatus vsm_shutdown_domain(VsmClient client, const char* id);

/**
 * Start domain
 *
 * @param[in] client security-containers-server's client
 * @param[in] id domain name
 * @return status of this function call
 */
VsmStatus vsm_start_domain(VsmClient client, const char* id);

/**
 * Lock domain
 *
 * @param[in] client security-containers-server's client
 * @param[in] id domain name
 * @return status of this function call
 */
VsmStatus vsm_lock_domain(VsmClient client, const char* id);

/**
 * Unlock domain
 *
 * @param[in] client security-containers-server's client
 * @param[in] id domain name
 * @return status of this function call
 */
VsmStatus vsm_unlock_domain(VsmClient client, const char* id);

/**
 * Register dbus state change callback function.
 *
 * @note The callback function will be invoked on a different thread.
 *
 * @param[in] client security-containers-server's client
 * @param[in] containerDbusStateCallback callback function
 * @param[in] data some extra data that will be passed to callback function
 * @param[out] subscriptionId subscription identifier that can be used to unsubscribe signal,
 *                      pointer can be NULL.
 * @return status of this function call
 */
VsmStatus vsm_add_state_callback(VsmClient client,
                                 VsmContainerDbusStateCallback containerDbusStateCallback,
                                 void* data,
                                 VsmSubscriptionId* subscriptionId);

/**
 * Unregister dbus state change callback function.
 *
 * @param[in] client security-containers-server's client
 * @param[in] subscriptionId subscription identifier returned by vsm_add_state_callback
 * @return status of this function call
 */
VsmStatus vsm_del_state_callback(VsmClient client, VsmSubscriptionId subscriptionId);

/**
 * Grant access to device
 *
 * @param[in] client security-containers-server's client
 * @param[in] domain domain name
 * @param[in] device device path
 * @param[in] flags access flags
 * @return status of this function call
 */
VsmStatus vsm_domain_grant_device(VsmClient client,
                                  const char* domain,
                                  const char* device,
                                  uint32_t flags);

/**
 * Revoke access to device
 *
 * @param[in] client security-containers-server's client
 * @param[in] domain domain name
 * @param[in] device device path
 * @return status of this function call
 */
VsmStatus vsm_revoke_device(VsmClient client, const char* domain, const char* device);

/**
 * Get array of netdev from given domain
 *
 * @param[in] client security-containers-server's client
 * @param[in] domain domain name
 * @param[out] netdevIds array of netdev id
 * @return status of this function call
 * @remark Use vsm_array_string_free() to free memory occupied by @p netdevIds.
 */
VsmStatus vsm_domain_get_netdevs(VsmClient client, const char* domain, VsmArrayString* netdevIds);

/**
 * Get ipv4 address for given netdevId
 *
 * @param[in] client security-containers-server's client
 * @param[in] domain domain name
 * @param[in] netdevId netdev id
 * @param[out] addr ipv4 address
 * @return status of this function call
 */
VsmStatus vsm_netdev_get_ipv4_addr(VsmClient client,
                                   const char* domain,
                                   const char* netdevId,
                                   struct in_addr *addr);

/**
 * Get ipv6 address for given netdevId
 *
 * @param[in] client security-containers-server's client
 * @param[in] domain domain name
 * @param[in] netdevId netdev id
 * @param[out] addr ipv6 address
 * @return status of this function call
 */
VsmStatus vsm_netdev_get_ipv6_addr(VsmClient client,
                                   const char* domain,
                                   const char* netdevId,
                                   struct in6_addr *addr);

/**
 * Set ipv4 address for given netdevId
 *
 * @param[in] client security-containers-server's client
 * @param[in] domain domain name
 * @param[in] netdevId netdev id
 * @param[in] addr ipv4 address
 * @param[in] prefix bit-length of the network prefix
 * @return status of this function call
 */
VsmStatus vsm_netdev_set_ipv4_addr(VsmClient client,
                                   const char* domain,
                                   const char* netdevId,
                                   struct in_addr *addr,
                                   int prefix);

/**
 * Set ipv6 address for given netdevId
 *
 * @param[in] client security-containers-server's client
 * @param[in] domain domain name
 * @param[in] netdevId netdev id
 * @param[in] addr ipv6 address
 * @param[in] prefix bit-length of the network prefix
 * @return status of this function call
 */
VsmStatus vsm_netdev_set_ipv6_addr(VsmClient client,
                                   const char* domain,
                                   const char* netdevId,
                                   struct in6_addr *addr,
                                   int prefix);

/**
 * Create netdev in domain
 *
 * @param[in] client security-containers-server's client
 * @param[in] domain domain name
 * @param[in] netdevType netdev type
 * @param[in] target TODO: this is taken form domain-control
 * @param[in] netdevId network device id
 * @return status of this function call
 */
VsmStatus vsm_create_netdev(VsmClient client,
                            const char* domain,
                            VsmNetdevType netdevType,
                            const char* target,
                            const char* netdevId);

/**
 * Remove netdev from domain
 *
 * @param[in] client security-containers-server's client
 * @param[in] domain domain name
 * @param[in] netdevId network device id
 * @return status of this function call
 */
VsmStatus vsm_destroy_netdev(VsmClient client, const char* domain, const char* netdevId);

/**
 * Get netdev informations
 *
 * @param[in] client security-containers-server's client
 * @param[in] domain domain name
 * @param[in] netdevId network device id
 * @param[out] netdev netdev informations
 * @return status of this function call
 * @remark Use vsm_netdev_free() to free memory occupied by @p netdev.
 */
VsmStatus vsm_lookup_netdev_by_name(VsmClient client,
                                    const char* domain,
                                    const char* netdevId,
                                    VsmNetdev* netdev);


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
 * @param data custom user's data pointer passed to vsm_add_notification_callback()
 */
typedef void (*VsmNotificationCallback)(const char* container,
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
VsmStatus vsm_notify_active_container(VsmClient client, const char* application, const char* message);

/**
 * Move file between containers.
 *
 * @param[in] client security-containers-server's client
 * @param[in] destContainer destination container id
 * @param[in] path path to moved file
 * @return status of this function call
 */
VsmStatus vsm_file_move_request(VsmClient client, const char* destContainer, const char* path);

/**
 * Register notification callback function.
 *
 * @note The callback function will be invoked on a different thread.
 *
 * @param[in] client security-containers-server's client
 * @param[in] notificationCallback callback function
 * @param[in] data some extra data that will be passed to callback function
 * @param[out] subscriptionId subscription identifier that can be used to unsubscribe signal,
 *                      pointer can be NULL.
 * @return status of this function call
 */
VsmStatus vsm_add_notification_callback(VsmClient client,
                                        VsmNotificationCallback notificationCallback,
                                        void* data,
                                        VsmSubscriptionId* subscriptionId);

/**
 * Unregister notification callback function.
 *
 * @param[in] client security-containers-server's client
 * @param[in] subscriptionId subscription identifier returned by vsm_add_notification_callback
 * @return status of this function call
 */
VsmStatus vsm_del_notification_callback(VsmClient client, VsmSubscriptionId subscriptionId);

/** @} */ // Domain API

#ifdef __cplusplus
}
#endif

#endif /* SECRITY_CONTAINERS_CLIENT_H */
