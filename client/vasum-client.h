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
 * @brief   This file contains the public API for Vasum Client
 *
 * @par Example usage:
 * @code
#include <stdio.h>
#include "client/vasum-client.h"

int main(int argc, char** argv)
{
    VsmStatus status;
    VsmClient client;
    VsmArrayString values = NULL;
    int ret = 0;

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

    status = vsm_get_zone_ids(client, &values);
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
    return ret;
}
 @endcode
 */

#ifndef VASUM_CLIENT_H
#define VASUM_CLIENT_H

#include <stdint.h>
#include <sys/stat.h>
#include <netinet/ip.h>
#include <linux/if_link.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * vasum-server's client pointer.
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
    VSMCLIENT_CUSTOM_ERROR,     /**< User specified error */
    VSMCLIENT_IO_ERROR,         /**< Input/Output error */
    VSMCLIENT_OPERATION_FAILED, /**< Operation failed */
    VSMCLIENT_INVALID_ARGUMENT, /**< Invalid argument */
    VSMCLIENT_OTHER_ERROR,      /**< Other error */
    VSMCLIENT_SUCCESS           /**< Success */
} VsmStatus;

/**
 * Subscription id
 */
typedef unsigned int VsmSubscriptionId;

/**
 * States of zone
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
} VsmZoneState;

/**
 * Zone information structure
 */
typedef struct {
    char* id;
    int terminal;
    VsmZoneState state;
    char *rootfs_path;
} VsmZoneStructure;

/**
 * Zone information
 */
typedef VsmZoneStructure* VsmZone;

/**
 * Netowrk device type
 */
typedef enum {
    VSMNETDEV_VETH,
    VSMNETDEV_PHYS,
    VSMNETDEV_MACVLAN
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
 * File type
 */
typedef enum {
    VSMFILE_DIRECTORY,
    VSMFILE_FIFO,
    VSMFILE_REGULAR
} VsmFileType;

/**
 * Event dispacher types.
 *
 * @par Example usage:
 * @code
#include <pthread.h>
#include <assert.h>
#include <stdio.h>
#include <signal.h>
#include <sys/epoll.h>
#include <vasum-client.h>

volatile static sig_atomic_t running;
static int LOOP_INTERVAL = 1000; // ms

void* main_loop(void* client)
{
    int fd = -1;
    VsmStatus status = vsm_get_poll_fd(client, &fd);
    assert(VSMCLIENT_SUCCESS == status);

    while (running) {
        struct epoll_event event;
        int num = epoll_wait(fd, &event, 1, LOOP_INTERVAL);
        if (num > 0) {
            status = vsm_enter_eventloop(client, 0 , 0);
            assert(VSMCLIENT_SUCCESS == status);
        }
    }
    return NULL;
}

int main(int argc, char** argv)
{
    pthread_t loop;
    VsmStatus status;
    VsmClient client;
    int ret = 0;

    client = vsm_client_create();
    assert(client);

    status = vsm_set_dispatcher_type(client, VSMDISPATCHER_EXTERNAL);
    assert(VSMCLIENT_SUCCESS == status);

    status = vsm_connect(client);
    assert(VSMCLIENT_SUCCESS == status);

    // start event loop
    running = 1;
    ret = pthread_create(&loop, NULL, main_loop, client);
    assert(ret == 0);

    // make vsm_* calls on client
    // ...

    status = vsm_disconnect(client);
    assert(VSMCLIENT_SUCCESS == status);

    //stop event loop
    running = 0;
    ret = pthread_join(loop, NULL);
    assert(ret == 0);

    vsm_client_free(client); // destroy client handle
    return ret;
}
 @endcode
 */
typedef enum {
    VSMDISPATCHER_EXTERNAL,         /**< User must handle dispatching messages */
    VSMDISPATCHER_INTERNAL          /**< Library will take care of dispatching messages */
} VsmDispacherType;

#ifndef __VASUM_WRAPPER_SOURCE__

/**
 * Get file descriptor associated with event dispatcher of zone client
 *
 * The function vsm_get_poll_fd() returns the file descriptor associated with the event dispatcher of vsm client.
 * The file descriptor can be bound to another I/O multiplexing facilities like epoll, select, and poll.
 *
 * @param[in] client vsm client
 * @param[out] fd epoll file descriptor
 * @return status of this function call
*/
VsmStatus vsm_get_poll_fd(VsmClient client, int* fd);

/**
 * Wait for an I/O event on a vsm client
 *
 * vsm_enter_eventloop() waits for event on the vsm client
 *
 * The call waits for a maximum time of timout milliseconds. Specifying a timeout of -1 makes vsm_enter_eventloop() wait indefinitely,
 * while specifying a timeout equal to zero makes vsm_enter_eventloop() to return immediately even if no events are available.
 *
 * @param[in] client vasum-server's client
 * @param[in] flags Reserved
 * @param[in] timeout Timeout time (milisecond), -1 is infinite
 * @return status of this function call
*/
VsmStatus vsm_enter_eventloop(VsmClient client, int flags, int timeout);

/**
 * Set dispatching method
 *
 * @param[in] client vasum-server's client
 * @param[in] dispacher dispatching method
 * @return status of this function call
 */
VsmStatus vsm_set_dispatcher_type(VsmClient client, VsmDispacherType dispacher);

/**
 * Get dispatching method
 *
 * @param[in] client vasum-server's client
 * @param[out] dispacher dispatching method
 * @return status of this function call
 */
VsmStatus vsm_get_dispatcher_type(VsmClient client, VsmDispacherType* dispacher);

/**
 * Create a new vasum-server's client.
 *
 * @return Created client pointer or NULL on failure.
 */
VsmClient vsm_client_create();

/**
 * Release client resources.
 *
 * @param[in] client vasum-server's client
 */
void vsm_client_free(VsmClient client);

/**
 * Get status code of last vasum-server communication.
 *
 * @param[in] client vasum-server's client
 * @return status of this function call
 */
VsmStatus vsm_get_status(VsmClient client);

/**
 * Get status message of the last vasum-server communication.
 *
 * @param[in] client vasum-server's client
 * @return last status message from vasum-server communication
 */
const char* vsm_get_status_message(VsmClient client);

/**
 * Connect client to the vasum-server.
 *
 * @param[in] client vasum-server's client
 * @return status of this function call
 */
VsmStatus vsm_connect(VsmClient client);

/**
 * Connect client to the vasum-server via custom address.
 *
 * @param[in] client vasum-server's client
 * @param[in] address dbus address
 * @return status of this function call
 */
VsmStatus vsm_connect_custom(VsmClient client, const char* address);

/**
 * Disconnect client from vasum-server.
 *
 * @param[in] client vasum-server's client
 * @return status of this function call
 */
VsmStatus vsm_disconnect(VsmClient client);

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
 * Release VsmZone
 *
 * @param zone VsmZone
 */
void vsm_zone_free(VsmZone zone);

/**
 * Release VsmNetdev
 *
 * @param netdev VsmNetdev
 */
void vsm_netdev_free(VsmNetdev netdev);

/**
 * @name Host API
 *
 * Functions using org.tizen.vasum.host.manager D-Bus interface.
 *
 * @{
 */

/**
 * Zone's D-Bus state change callback function signature.
 *
 * @param[in] zoneId affected zone id
 * @param[in] address new D-Bus address
 * @param data custom user's data pointer passed to vsm_add_state_callback() function
 */
typedef void (*VsmZoneDbusStateCallback)(const char* zoneId,
                                             const char* address,
                                             void* data);

/**
 * Lock the command queue exclusively.
 *
 * @param[in] client vasum-server's client
 * @return status of this function call
 */
VsmStatus vsm_lock_queue(VsmClient client);

/**
 * Unlock the command queue.
 *
 * @param[in] client vasum-server's client
 * @return status of this function call
 */
VsmStatus vsm_unlock_queue(VsmClient client);

/**
 * Get dbus address of each zone.
 *
 * @param[in] client vasum-server's client
 * @param[out] keys array of zones name
 * @param[out] values array of zones dbus address
 * @return status of this function call
 * @post keys[i] corresponds to values[i]
 * @remark Use vsm_array_string_free() to free memory occupied by @p keys and @p values.
 */
VsmStatus vsm_get_zone_dbuses(VsmClient client, VsmArrayString* keys, VsmArrayString* values);

/**
 * Get zones name.
 *
 * @param[in] client vasum-server's client
 * @param[out] array array of zones name
 * @return status of this function call
 * @remark Use vsm_array_string_free() to free memory occupied by @p array.
 */
VsmStatus vsm_get_zone_ids(VsmClient client, VsmArrayString* array);

/**
 * Get active (foreground) zone name.
 *
 * @param[in] client vasum-server's client
 * @param[out] id active zone name
 * @return status of this function call
 * @remark Use @p vsm_string_free() to free memory occupied by @p id.
 */
VsmStatus vsm_get_active_zone_id(VsmClient client, VsmString* id);

/**
 * Get zone rootfs path.
 *
 * @param[in] client vasum-server's client
 * @param[in] id zone name
 * @param[out] rootpath zone rootfs path
 * @return status of this function call
 * @remark Use @p vsm_string_free() to free memory occupied by @p rootpath.
 */
VsmStatus vsm_get_zone_rootpath(VsmClient client, const char* id, VsmString* rootpath);

/**
 * Get zone name of process with given pid.
 *
 * @param[in] client vasum-server's client
 * @param[in] pid process id
 * @param[out] id active zone name
 * @return status of this function call
 * @remark Use @p vsm_string_free() to free memory occupied by @p id.
 */
VsmStatus vsm_lookup_zone_by_pid(VsmClient client, int pid, VsmString* id);

/**
 * Get zone informations of zone with given id.
 *
 * @param[in] client vasum-server's client
 * @param[in] id zone name
 * @param[out] zone zone informations
 * @return status of this function call
 * @remark Use @p vsm_zone_free() to free memory occupied by @p zone
 */
VsmStatus vsm_lookup_zone_by_id(VsmClient client, const char* id, VsmZone* zone);

/**
 * Get zone name with given terminal.
 *
 * @param[in] client vasum-server's client
 * @param[in] terminal terminal id
 * @param[out] id zone name with given terminal
 * @return status of this function call
 * @remark Use @p vsm_string_free() to free memory occupied by @p id.
 */
VsmStatus vsm_lookup_zone_by_terminal_id(VsmClient client, int terminal, VsmString* id);

/**
 * Set active (foreground) zone.
 *
 * @param[in] client vasum-server's client
 * @param[in] id zone name
 * @return status of this function call
 */
VsmStatus vsm_set_active_zone(VsmClient client, const char* id);

/**
 * Create and add zone
 *
 * @param[in] client vasum-server's client
 * @param[in] id zone id
 * @param[in] tname template name, NULL is equivalent to "default"
 * @return status of this function call
 */
VsmStatus vsm_create_zone(VsmClient client, const char* id, const char* tname);

/**
 * Remove zone
 *
 * @param[in] client vasum-server's client
 * @param[in] id zone id
 * @param[in] force if 0 data will be kept, otherwise data will be lost
 * @return status of this function call
 */
VsmStatus vsm_destroy_zone(VsmClient client, const char* id, int force);

/**
 * Shutdown zone
 *
 * @param[in] client vasum-server's client
 * @param[in] id zone name
 * @return status of this function call
 */
VsmStatus vsm_shutdown_zone(VsmClient client, const char* id);

/**
 * Start zone
 *
 * @param[in] client vasum-server's client
 * @param[in] id zone name
 * @return status of this function call
 */
VsmStatus vsm_start_zone(VsmClient client, const char* id);

/**
 * Lock zone
 *
 * @param[in] client vasum-server's client
 * @param[in] id zone name
 * @return status of this function call
 */
VsmStatus vsm_lock_zone(VsmClient client, const char* id);

/**
 * Unlock zone
 *
 * @param[in] client vasum-server's client
 * @param[in] id zone name
 * @return status of this function call
 */
VsmStatus vsm_unlock_zone(VsmClient client, const char* id);

/**
 * Register dbus state change callback function.
 *
 * @note The callback function will be invoked on a different thread.
 *
 * @param[in] client vasum-server's client
 * @param[in] zoneDbusStateCallback callback function
 * @param[in] data some extra data that will be passed to callback function
 * @param[out] subscriptionId subscription identifier that can be used to unsubscribe signal,
 *                      pointer can be NULL.
 * @return status of this function call
 */
VsmStatus vsm_add_state_callback(VsmClient client,
                                 VsmZoneDbusStateCallback zoneDbusStateCallback,
                                 void* data,
                                 VsmSubscriptionId* subscriptionId);

/**
 * Unregister dbus state change callback function.
 *
 * @param[in] client vasum-server's client
 * @param[in] subscriptionId subscription identifier returned by vsm_add_state_callback
 * @return status of this function call
 */
VsmStatus vsm_del_state_callback(VsmClient client, VsmSubscriptionId subscriptionId);

/**
 * Grant access to device
 *
 * @param[in] client vasum-server's client
 * @param[in] zone zone name
 * @param[in] device device path
 * @param[in] flags access flags
 * @return status of this function call
 */
VsmStatus vsm_grant_device(VsmClient client,
                           const char* zone,
                           const char* device,
                           uint32_t flags);

/**
 * Revoke access to device
 *
 * @param[in] client vasum-server's client
 * @param[in] zone zone name
 * @param[in] device device path
 * @return status of this function call
 */
VsmStatus vsm_revoke_device(VsmClient client, const char* zone, const char* device);

/**
 * Get array of netdev from given zone
 *
 * @param[in] client vasum-server's client
 * @param[in] zone zone name
 * @param[out] netdevIds array of netdev id
 * @return status of this function call
 * @remark Use vsm_array_string_free() to free memory occupied by @p netdevIds.
 */
VsmStatus vsm_zone_get_netdevs(VsmClient client, const char* zone, VsmArrayString* netdevIds);

/**
 * Get ipv4 address for given netdevId
 *
 * @param[in] client vasum-server's client
 * @param[in] zone zone name
 * @param[in] netdevId netdev id
 * @param[out] addr ipv4 address
 * @return status of this function call
 */
VsmStatus vsm_netdev_get_ipv4_addr(VsmClient client,
                                   const char* zone,
                                   const char* netdevId,
                                   struct in_addr *addr);

/**
 * Get ipv6 address for given netdevId
 *
 * @param[in] client vasum-server's client
 * @param[in] zone zone name
 * @param[in] netdevId netdev id
 * @param[out] addr ipv6 address
 * @return status of this function call
 */
VsmStatus vsm_netdev_get_ipv6_addr(VsmClient client,
                                   const char* zone,
                                   const char* netdevId,
                                   struct in6_addr *addr);

/**
 * Set ipv4 address for given netdevId
 *
 * @param[in] client vasum-server's client
 * @param[in] zone zone name
 * @param[in] netdevId netdev id
 * @param[in] addr ipv4 address
 * @param[in] prefix bit-length of the network prefix
 * @return status of this function call
 */
VsmStatus vsm_netdev_set_ipv4_addr(VsmClient client,
                                   const char* zone,
                                   const char* netdevId,
                                   struct in_addr *addr,
                                   int prefix);

/**
 * Set ipv6 address for given netdevId
 *
 * @param[in] client vasum-server's client
 * @param[in] zone zone name
 * @param[in] netdevId netdev id
 * @param[in] addr ipv6 address
 * @param[in] prefix bit-length of the network prefix
 * @return status of this function call
 */
VsmStatus vsm_netdev_set_ipv6_addr(VsmClient client,
                                   const char* zone,
                                   const char* netdevId,
                                   struct in6_addr *addr,
                                   int prefix);

/**
 * Remove ipv4 address from netdev
 *
 * @param[in] client vasum-server's client
 * @param[in] zone zone name
 * @param[in] netdevId network device id
 * @param[in] addr ipv4 address
 * @param[in] prefix bit-length of the network prefix
 * @return status of this function call
 */
VsmStatus vsm_netdev_del_ipv4_addr(VsmClient client,
                                   const char* zone,
                                   const char* netdevId,
                                   struct in_addr* addr,
                                   int prefix);

/**
 * Remove ipv6 address from netdev
 *
 * @param[in] client vasum-server's client
 * @param[in] zone zone name
 * @param[in] netdevId network device id
 * @param[in] addr ipv6 address
 * @param[in] prefix bit-length of the network prefix
 * @return status of this function call
 */
VsmStatus vsm_netdev_del_ipv6_addr(VsmClient client,
                                   const char* zone,
                                   const char* netdevId,
                                   struct in6_addr* addr,
                                   int prefix);

/**
 * Turn up a network device in the zone
 *
 * @param[in] client vasum-server's client
 * @param[in] zone zone name
 * @param[in] netdevId netdev id
 * @return status of this function call
 */
VsmStatus vsm_netdev_up(VsmClient client,
                        const char* zone,
                        const char* netdevId);

/**
 * Turn down a network device in the zone
 *
 * @param[in] client vasum-server's client
 * @param[in] zone zone name
 * @param[in] netdevId netdev id
 * @return status of this function call
 */
VsmStatus vsm_netdev_down(VsmClient client,
                          const char* zone,
                          const char* netdevId);


/**
 * Create veth netdev in zone
 *
 * @param[in] client vasum-server's client
 * @param[in] zone zone name
 * @param[in] zoneDev in host network device id
 * @param[in] hostDev in zone network device id
 * @return status of this function call
 */
VsmStatus vsm_create_netdev_veth(VsmClient client,
                                 const char* zone,
                                 const char* zoneDev,
                                 const char* hostDev);
/**
 * Create macvlab in zone
 *
 * @param[in] client vasum-server's client
 * @param[in] zone zone name
 * @param[in] zoneDev in host network device id
 * @param[in] hostDev in zone network device id
 * @return status of this function call
 */
VsmStatus vsm_create_netdev_macvlan(VsmClient client,
                                    const char* zone,
                                    const char* zoneDev,
                                    const char* hostDev,
                                    enum macvlan_mode mode);
/**
 * Create/move phys netdev in/to zone
 *
 * @param[in] client vasum-server's client
 * @param[in] zone zone name
 * @param[in] devId network device id
 * @return status of this function call
 */
VsmStatus vsm_create_netdev_phys(VsmClient client, const char* zone, const char* devId);

/**
 * Get netdev informations
 *
 * @param[in] client vasum-server's client
 * @param[in] zone zone name
 * @param[in] netdevId network device id
 * @param[out] netdev netdev informations
 * @return status of this function call
 * @remark Use vsm_netdev_free() to free memory occupied by @p netdev.
 */
VsmStatus vsm_lookup_netdev_by_name(VsmClient client,
                                    const char* zone,
                                    const char* netdevId,
                                    VsmNetdev* netdev);

/**
 * Remove netdev from zone
 *
 * @param[in] client vasum-server's client
 * @param[in] zone zone name
 * @param[in] devId network device id
 * @return status of this function call
 */
VsmStatus vsm_destroy_netdev(VsmClient client, const char* zone, const char* devId);

/**
 * Create file, directory or pipe in zone
 *
 * Declare file, directory or pipe that will be created while zone startup
 *
 * @param[in] client vasum-server's client
 * @param[in] type file type
 * @param[in] zone zone id
 * @param[in] path path to file
 * @param[in] flags if O_CREAT bit is set then file will be created in zone,
 *                  otherwise file will by copied from host;
 *                  it is meaningful only when O_CREAT is set
 * @param[in] mode mode of file
 * @return status of this function call
 */
VsmStatus vsm_declare_file(VsmClient client,
                           const char* zone,
                           VsmFileType type,
                           const char* path,
                           int32_t flags,
                           mode_t mode);

/**
 * Create mount point in zone
 *
 * Declare mount that will be created while zone startup
 * Parameters are passed to mount system function
 *
 * @param[in] client vasum-server's client
 * @param[in] source device path (path in host)
 * @param[in] zone zone id
 * @param[in] target mount point (path in zone)
 * @param[in] type filesystem type
 * @param[in] flags mount flags as in mount function
 * @param[in] data additional data as in mount function
 * @return status of this function call
 */
VsmStatus vsm_declare_mount(VsmClient client,
                            const char* source,
                            const char* zone,
                            const char* target,
                            const char* type,
                            uint64_t flags,
                            const char* data);

/**
 * Create link in zone
 *
 * Declare link that will be created while zone startup
 * Parameters are passed to link system function
 *
 * @param[in] client vasum-server's client
 * @param[in] source path to link source (in host)
 * @param[in] zone zone id
 * @param[in] target path to link name (in zone)
 * @return status of this function call
 */
VsmStatus vsm_declare_link(VsmClient client,
                           const char *source,
                           const char* zone,
                           const char *target);

/**
 * Get all declarations
 *
 * Gets all declarations of resourcies
 * (@see ::vsm_declare_link, @see ::vsm_declare_mount, @see ::vsm_declare_linki)
 *
 * @param[in] client vasum-server's client
 * @param[in] zone zone id
 * @param[out] declarations array of declarations id
 * @return status of this function call
 */
VsmStatus vsm_list_declarations(VsmClient client,
                                const char* zone,
                                VsmArrayString* declarations);

/**
 * Remove declaration
 *
 * Removes given declaration by its id (@see ::vsm_list_declarations)
 *
 * @param[in] client vasum-server's client
 * @param[in] zone zone id
 * @param[in] declaration declaration id
 * @return status of this function call
 */
VsmStatus vsm_remove_declaration(VsmClient client,
                                 const char* zone,
                                 VsmString declaration);


/** @} Host API */


#endif /* __VASUM_WRAPPER_SOURCE__ */

#ifdef __cplusplus
}
#endif

#endif /* VASUM_CLIENT_H */
