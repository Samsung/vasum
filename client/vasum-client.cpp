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
 */

#include "config.hpp"
#include "vasum-client.h"
#include "vasum-client-impl.hpp"

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

API VsmStatus vsm_lock_queue(VsmClient client)
{
    return getClient(client).vsm_lock_queue();
}

API VsmStatus vsm_unlock_queue(VsmClient client)
{
    return getClient(client).vsm_unlock_queue();
}

API VsmStatus vsm_get_poll_fd(VsmClient client, int* fd)
{
    return getClient(client).vsm_get_poll_fd(fd);
}

API VsmStatus vsm_enter_eventloop(VsmClient client, int flags, int timeout)
{
    return getClient(client).vsm_enter_eventloop(flags, timeout);
}

API VsmStatus vsm_set_dispatcher_type(VsmClient client, VsmDispacherType dispacher)
{
    return getClient(client).vsm_set_dispatcher_type(dispacher);
}

API VsmStatus vsm_get_dispatcher_type(VsmClient client, VsmDispacherType* dispacher)
{
    return getClient(client).vsm_get_dispatcher_type(dispacher);
}

API VsmClient vsm_client_create()
{
    Client* clientPtr = new(nothrow) Client();
    return reinterpret_cast<VsmClient>(clientPtr);
}

API VsmStatus vsm_connect(VsmClient client)
{
    return getClient(client).connectSystem();
}

API VsmStatus vsm_connect_custom(VsmClient client, const char* address)
{
    return getClient(client).connect(address);
}

API VsmStatus vsm_disconnect(VsmClient client)
{
    return getClient(client).disconnect();
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

API VsmString vsm_zone_get_id(VsmZone zone)
{
    Zone z = static_cast<Zone>(zone);
    return z->id;
}

API int vsm_zone_get_terminal(VsmZone zone)
{
    Zone z = static_cast<Zone>(zone);
    return z->terminal;
}

API VsmZoneState vsm_zone_get_state(VsmZone zone)
{
    Zone z = static_cast<Zone>(zone);
    return z->state;
}

API VsmString vsm_zone_get_rootfs(VsmZone zone)
{
    Zone z = static_cast<Zone>(zone);
    return z->rootfs_path;
}

API void vsm_zone_free(VsmZone zone)
{
    Zone z = static_cast<Zone>(zone);
    free(z->rootfs_path);
    free(z->id);
    free(z);
}

API VsmString vsm_netdev_get_name(VsmNetdev netdev)
{
    Netdev n = static_cast<Netdev>(netdev);
    return n->name;
}

API VsmNetdevType vsm_netdev_get_type(VsmNetdev netdev)
{
    Netdev n = static_cast<Netdev>(netdev);
    return n->type;
}

API void vsm_netdev_free(VsmNetdev netdev)
{
    Netdev n = static_cast<Netdev>(netdev);
    free(n->name);
    free(n);
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

API VsmStatus vsm_get_zone_dbuses(VsmClient client, VsmArrayString* keys, VsmArrayString* values)
{
    return getClient(client).vsm_get_zone_dbuses(keys, values);
}

API VsmStatus vsm_get_zone_ids(VsmClient client, VsmArrayString* array)
{
    return getClient(client).vsm_get_zone_ids(array);
}

API VsmStatus vsm_get_active_zone_id(VsmClient client, VsmString* id)
{
    return getClient(client).vsm_get_active_zone_id(id);
}

API VsmStatus vsm_lookup_zone_by_pid(VsmClient client, int pid, VsmString* id)
{
    return getClient(client).vsm_lookup_zone_by_pid(pid, id);
}

API VsmStatus vsm_lookup_zone_by_id(VsmClient client, const char* id, VsmZone* zone)
{
    Zone* z = reinterpret_cast<Zone*>(zone);
    return getClient(client).vsm_lookup_zone_by_id(id, z);
}

API VsmStatus vsm_lookup_zone_by_terminal_id(VsmClient client, int terminal, VsmString* id)
{
    return getClient(client).vsm_lookup_zone_by_terminal_id(terminal, id);
}

API VsmStatus vsm_set_active_zone(VsmClient client, const char* id)
{
    return getClient(client).vsm_set_active_zone(id);
}

API VsmStatus vsm_create_zone(VsmClient client, const char* id, const char* tname)
{
    return getClient(client).vsm_create_zone(id, tname);
}

API VsmStatus vsm_destroy_zone(VsmClient client, const char* id, int /*force*/)
{
    return getClient(client).vsm_destroy_zone(id);
}

API VsmStatus vsm_shutdown_zone(VsmClient client, const char* id)
{
    return getClient(client).vsm_shutdown_zone(id);
}

API VsmStatus vsm_start_zone(VsmClient client, const char* id)
{
    return getClient(client).vsm_start_zone(id);
}

API VsmStatus vsm_lock_zone(VsmClient client, const char* id)
{
    return getClient(client).vsm_lock_zone(id);
}

API VsmStatus vsm_unlock_zone(VsmClient client, const char* id)
{
    return getClient(client).vsm_unlock_zone(id);
}

API VsmStatus vsm_add_state_callback(VsmClient client,
                                     VsmZoneDbusStateCallback zoneDbusStateCallback,
                                     void* data,
                                     VsmSubscriptionId* subscriptionId)
{
    return getClient(client).vsm_add_state_callback(zoneDbusStateCallback, data, subscriptionId);
}

API VsmStatus vsm_del_state_callback(VsmClient client, VsmSubscriptionId subscriptionId)
{
    return getClient(client).vsm_del_state_callback(subscriptionId);
}

API VsmStatus vsm_grant_device(VsmClient client,
                               const char* id,
                               const char* device,
                               uint32_t flags)
{
    return getClient(client).vsm_grant_device(id, device, flags);
}

API VsmStatus vsm_revoke_device(VsmClient client, const char* id, const char* device)
{
    return getClient(client).vsm_revoke_device(id, device);
}

API VsmStatus vsm_zone_get_netdevs(VsmClient client,
                                     const char* zone,
                                     VsmArrayString* netdevIds)
{
    return getClient(client).vsm_zone_get_netdevs(zone, netdevIds);
}

API VsmStatus vsm_netdev_get_ip_addr(VsmClient client,
                                 const char* zone,
                                 const char* netdevId,
                                 VsmAddrList *addrs)
{
    std::vector<InetAddr> addrlist;
    VsmStatus status = getClient(client).vsm_netdev_get_ip_addr(zone, netdevId, addrlist);
    int n = addrlist.size();
    InetAddr *a = (InetAddr *)malloc((n+1)*sizeof(InetAddr));
    std::copy(addrlist.begin(), addrlist.end(), a);
    a[n].type=-1;
    *addrs = a;
    return status;
}

API VsmStatus vsm_netdev_get_ipv4_addr(VsmClient client,
                                       const char* zone,
                                       const char* netdevId,
                                       struct in_addr *addr)
{
    return getClient(client).vsm_netdev_get_ipv4_addr(zone, netdevId, addr);
}

API VsmStatus vsm_netdev_get_ipv6_addr(VsmClient client,
                                       const char* zone,
                                       const char* netdevId,
                                       struct in6_addr *addr)
{
    return getClient(client).vsm_netdev_get_ipv6_addr(zone, netdevId, addr);
}

API VsmStatus vsm_netdev_add_ipv4_addr(VsmClient client,
                                       const char* zone,
                                       const char* netdevId,
                                       struct in_addr *addr,
                                       int prefix)
{
    return getClient(client).vsm_netdev_add_ipv4_addr(zone, netdevId, addr, prefix);
}

API VsmStatus vsm_netdev_add_ipv6_addr(VsmClient client,
                                       const char* zone,
                                       const char* netdevId,
                                       struct in6_addr *addr,
                                       int prefix)
{
    return getClient(client).vsm_netdev_add_ipv6_addr(zone, netdevId, addr, prefix);
}

API VsmStatus vsm_netdev_del_ipv4_addr(VsmClient client,
                                       const char* zone,
                                       const char* netdevId,
                                       struct in_addr* addr,
                                       int prefix)
{
    return getClient(client).vsm_netdev_del_ipv4_addr(zone, netdevId, addr, prefix);
}

API VsmStatus vsm_netdev_del_ipv6_addr(VsmClient client,
                                       const char* zone,
                                       const char* netdevId,
                                       struct in6_addr* addr,
                                       int prefix)
{
    return getClient(client).vsm_netdev_del_ipv6_addr(zone, netdevId, addr, prefix);
}

API VsmStatus vsm_netdev_up(VsmClient client,
                            const char* zone,
                            const char* netdevId)
{
    return getClient(client).vsm_netdev_up(zone, netdevId);
}

API VsmStatus vsm_netdev_down(VsmClient client,
                              const char* zone,
                              const char* netdevId)
{
    return getClient(client).vsm_netdev_down(zone, netdevId);
}

API VsmStatus vsm_create_netdev_veth(VsmClient client,
                                     const char* zone,
                                     const char* zoneDev,
                                     const char* hostDev)
{
    return getClient(client).vsm_create_netdev_veth(zone, zoneDev, hostDev);
}

API VsmStatus vsm_create_netdev_macvlan(VsmClient client,
                                        const char* zone,
                                        const char* zoneDev,
                                        const char* hostDev,
                                        enum macvlan_mode mode)
{
    return getClient(client).vsm_create_netdev_macvlan(zone, zoneDev, hostDev, mode);
}

API VsmStatus vsm_create_netdev_phys(VsmClient client, const char* zone, const char* devId)
{
    return getClient(client).vsm_create_netdev_phys(zone, devId);
}

API VsmStatus vsm_lookup_netdev_by_name(VsmClient client,
                                        const char* zone,
                                        const char* netdevId,
                                        VsmNetdev* netdev)
{
    Netdev* n = reinterpret_cast<Netdev*>(netdev);
    return getClient(client).vsm_lookup_netdev_by_name(zone, netdevId, n);
}

API VsmStatus vsm_destroy_netdev(VsmClient client, const char* zone, const char* devId)
{
    return getClient(client).vsm_destroy_netdev(zone, devId);
}

API VsmStatus vsm_declare_file(VsmClient client,
                               const char* zone,
                               VsmFileType type,
                               const char* path,
                               int32_t flags,
                               mode_t mode)
{
    return getClient(client).vsm_declare_file(zone, type, path, flags, mode, NULL);
}


API VsmStatus vsm_declare_mount(VsmClient client,
                                const char* source,
                                const char* zone,
                                const char* target,
                                const char* type,
                                uint64_t flags,
                                const char* data)
{
    return getClient(client).vsm_declare_mount(source, zone, target, type, flags, data, NULL);
}

API VsmStatus vsm_declare_link(VsmClient client,
                               const char* source,
                               const char* zone,
                               const char* target)
{
    return getClient(client).vsm_declare_link(source, zone, target, NULL);
}

API VsmStatus vsm_list_declarations(VsmClient client,
                                    const char* zone,
                                    VsmArrayString* declarations)
{
    return getClient(client).vsm_list_declarations(zone, declarations);
}

API VsmStatus vsm_remove_declaration(VsmClient client,
                                     const char* zone,
                                     VsmString declaration)
{
    return getClient(client).vsm_remove_declaration(zone, declaration);
}

API VsmStatus vsm_clean_up_zones_root(VsmClient client)
{
    return getClient(client).vsm_clean_up_zones_root();
}

API unsigned int vsm_addrlist_size(VsmAddrList addrs)
{
    InetAddr *a = static_cast<InetAddr*>(addrs);
    unsigned int i;
    for (i = 0; a[i].type >= 0; ++i) ;
    return i;
}

API int vsm_addrlist_get_type(VsmAddrList addrs, unsigned int i)
{
    return static_cast<InetAddr*>(addrs)[i].type;
}

API const void *vsm_addrlist_get_addr(VsmAddrList addrs, unsigned int i)
{
    return &static_cast<InetAddr*>(addrs)[i].addr;
}

API unsigned int vsm_addrlist_get_prefix(VsmAddrList addrs, unsigned int i)
{
    return static_cast<InetAddr*>(addrs)[i].prefix;
}

API void vsm_addrlist_free(VsmAddrList addrs) {
    free(addrs);
}
