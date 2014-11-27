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

Client& getClient(VsmClient client)
{
    assert(client);
    return *reinterpret_cast<Client*>(client);
}

} // namespace

/* external */
API VsmStatus vsm_start_glib_loop()
{
    return Client::vsm_start_glib_loop();
}

API VsmStatus vsm_stop_glib_loop()
{
    return Client::vsm_stop_glib_loop();
}

API VsmClient vsm_client_create()
{
    Client* clientPtr = new(nothrow) Client();
    return reinterpret_cast<VsmClient>(clientPtr);
}

API VsmStatus vsm_connect(VsmClient client)
{
    return getClient(client).createSystem();
}

API VsmStatus vsm_connect_custom(VsmClient client, const char* address)
{
    return getClient(client).create(address);
}

API void vsm_array_string_free(VsmArrayString astring)
{
    if (!astring) {
        return;
    }
    for (char** ptr = astring; *ptr; ++ptr) {
        vsm_string_free(*ptr);
    }
    free(astring);
}

API void vsm_string_free(VsmString string)
{
    free(string);
}

API void vsm_domain_free(VsmDomain domain)
{
    free(domain->rootfs_path);
    free(domain->id);
    free(domain);
}

API void vsm_netdev_free(VsmNetdev netdev)
{
    free(netdev->name);
    free(netdev);
}

API void vsm_client_free(VsmClient client)
{
    if (client != NULL) {
        delete &getClient(client);
    }
}

API const char* vsm_get_status_message(VsmClient client)
{
    return getClient(client).vsm_get_status_message();
}

API VsmStatus vsm_get_status(VsmClient client)
{
    return getClient(client).vsm_get_status();
}

API VsmStatus vsm_get_container_dbuses(VsmClient client, VsmArrayString* keys, VsmArrayString* values)
{
    return getClient(client).vsm_get_container_dbuses(keys, values);
}

API VsmStatus vsm_get_domain_ids(VsmClient client, VsmArrayString* array)
{
    return getClient(client).vsm_get_domain_ids(array);
}

API VsmStatus vsm_get_active_container_id(VsmClient client, VsmString* id)
{
    return getClient(client).vsm_get_active_container_id(id);
}

API VsmStatus vsm_lookup_domain_by_pid(VsmClient client, int pid, VsmString* id)
{
    return getClient(client).vsm_lookup_domain_by_pid(pid, id);
}

API VsmStatus vsm_lookup_domain_by_id(VsmClient client, const char* id, VsmDomain* domain)
{
    return getClient(client).vsm_lookup_domain_by_id(id, domain);
}

API VsmStatus vsm_lookup_domain_by_terminal_id(VsmClient client, int terminal, VsmString* id)
{
    return getClient(client).vsm_lookup_domain_by_terminal_id(terminal, id);
}

API VsmStatus vsm_set_active_container(VsmClient client, const char* id)
{
    return getClient(client).vsm_set_active_container(id);
}

API VsmStatus vsm_create_domain(VsmClient client, const char* id, const char* tname)
{
    return getClient(client).vsm_create_domain(id, tname);
}

API VsmStatus vsm_destroy_domain(VsmClient client, const char* id, int /*force*/)
{
    return getClient(client).vsm_destroy_domain(id);
}

API VsmStatus vsm_shutdown_domain(VsmClient client, const char* id)
{
    return getClient(client).vsm_shutdown_domain(id);
}

API VsmStatus vsm_start_domain(VsmClient client, const char* id)
{
    return getClient(client).vsm_start_domain(id);
}

API VsmStatus vsm_lock_domain(VsmClient client, const char* id)
{
    return getClient(client).vsm_lock_domain(id);
}

API VsmStatus vsm_unlock_domain(VsmClient client, const char* id)
{
    return getClient(client).vsm_unlock_domain(id);
}

API VsmStatus vsm_add_state_callback(VsmClient client,
                                     VsmContainerDbusStateCallback containerDbusStateCallback,
                                     void* data,
                                     VsmSubscriptionId* subscriptionId)
{
    return getClient(client).vsm_add_state_callback(containerDbusStateCallback, data, subscriptionId);
}

API VsmStatus vsm_del_state_callback(VsmClient client, VsmSubscriptionId subscriptionId)
{
    return getClient(client).vsm_del_state_callback(subscriptionId);
}

API VsmStatus vsm_domain_grant_device(VsmClient client,
                                      const char* id,
                                      const char* device,
                                      uint32_t flags)
{
    return getClient(client).vsm_domain_grant_device(id, device, flags);
}

API VsmStatus vsm_revoke_device(VsmClient client, const char* id, const char* device)
{
    return getClient(client).vsm_revoke_device(id, device);
}

API VsmStatus vsm_domain_get_netdevs(VsmClient client,
                                     const char* domain,
                                     VsmArrayString* netdevIds)
{
    return getClient(client).vsm_domain_get_netdevs(domain, netdevIds);
}

API VsmStatus vsm_netdev_get_ipv4_addr(VsmClient client,
                                       const char* domain,
                                       const char* netdevId,
                                       struct in_addr *addr)
{
    return getClient(client).vsm_netdev_get_ipv4_addr(domain, netdevId, addr);
}

API VsmStatus vsm_netdev_get_ipv6_addr(VsmClient client,
                                       const char* domain,
                                       const char* netdevId,
                                       struct in6_addr *addr)
{
    return getClient(client).vsm_netdev_get_ipv6_addr(domain, netdevId, addr);
}

API VsmStatus vsm_netdev_set_ipv4_addr(VsmClient client,
                                       const char* domain,
                                       const char* netdevId,
                                       struct in_addr *addr,
                                       int prefix)
{
    return getClient(client).vsm_netdev_set_ipv4_addr(domain, netdevId, addr, prefix);
}

API VsmStatus vsm_netdev_set_ipv6_addr(VsmClient client,
                                       const char* domain,
                                       const char* netdevId,
                                       struct in6_addr *addr,
                                       int prefix)
{
    return getClient(client).vsm_netdev_set_ipv6_addr(domain, netdevId, addr, prefix);
}

API VsmStatus vsm_create_netdev(VsmClient client,
                                const char* domain,
                                VsmNetdevType netdevType,
                                const char* target,
                                const char* netdevId)
{
    return getClient(client).vsm_create_netdev(domain, netdevType, target, netdevId);
}

API VsmStatus vsm_destroy_netdev(VsmClient client, const char* domain, const char* netdevId)
{
    return getClient(client).vsm_destroy_netdev(domain, netdevId);
}

API VsmStatus vsm_lookup_netdev_by_name(VsmClient client,
                                        const char* domain,
                                        const char* netdevId,
                                        VsmNetdev* netdev)
{
    return getClient(client).vsm_lookup_netdev_by_name(domain, netdevId, netdev);
}

API VsmStatus vsm_notify_active_container(VsmClient client,
                                          const char* application,
                                          const char* message)
{
    return getClient(client).vsm_notify_active_container(application, message);
}

API VsmStatus vsm_file_move_request(VsmClient client, const char* destContainer, const char* path)
{
    return getClient(client).vsm_file_move_request(destContainer, path);
}

API VsmStatus vsm_add_notification_callback(VsmClient client,
                                            VsmNotificationCallback notificationCallback,
                                            void* data,
                                            VsmSubscriptionId* subscriptionId)
{
    return getClient(client).vsm_add_notification_callback(notificationCallback, data, subscriptionId);
}

API VsmStatus vsm_del_notification_callback(VsmClient client,
                                            VsmSubscriptionId subscriptionId)
{
    return getClient(client).vsm_del_notification_callback(subscriptionId);
}
