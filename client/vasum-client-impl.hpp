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
 * @brief   This file contains vasum-server's client definition
 */

#ifndef VASUM_CLIENT_IMPL_HPP
#define VASUM_CLIENT_IMPL_HPP

#include "vasum-client.h"
#ifdef DBUS_CONNECTION
#include "host-dbus-connection.hpp"
#else
#include "host-ipc-connection.hpp"
#endif
#include "zone-dbus-connection.hpp"

#include <functional>
#include <linux/if_link.h>

/**
 * vasum's client definition.
 *
 * Client uses dbus API.
 */
class Client {
public:
    Client() noexcept;
    ~Client() noexcept;

    /**
     * Create client with system dbus address.
     *
     * @return status of this function call
     */
    VsmStatus createSystem() noexcept;

    /**
     * Create client.
     *
     * @param address Dbus socket address
     * @return status of this function call
     */
    VsmStatus create(const std::string& address) noexcept;

    /**
     *  @see ::vsm_get_status_message
     */
    const char* vsm_get_status_message() const noexcept;

    /**
     *  @see ::vsm_get_status
     */
    VsmStatus vsm_get_status() const noexcept;

    /**
     *  @see ::vsm_get_zone_dbuses
     */
    VsmStatus vsm_get_zone_dbuses(VsmArrayString* keys, VsmArrayString* values) noexcept;

    /**
     *  @see ::vsm_get_zone_ids
     */
    VsmStatus vsm_get_zone_ids(VsmArrayString* array) noexcept;

    /**
     *  @see ::vsm_get_active_zone_id
     */
    VsmStatus vsm_get_active_zone_id(VsmString* id) noexcept;

    /**
     *  @see ::vsm_lookup_zone_by_pid
     */
    VsmStatus vsm_lookup_zone_by_pid(int pid, VsmString* id) noexcept;

    /**
     * @see ::vsm_lookup_zone_by_id
     */
    VsmStatus vsm_lookup_zone_by_id(const char* id, VsmZone* zone) noexcept;

    /**
     * @see ::vsm_lookup_zone_by_terminal_id
     */
    VsmStatus vsm_lookup_zone_by_terminal_id(int terminal, VsmString* id) noexcept;

    /**
     *  @see ::vsm_set_active_zone
     */
    VsmStatus vsm_set_active_zone(const char* id) noexcept;

    /**
     *  @see ::vsm_create_zone
     */
    VsmStatus vsm_create_zone(const char* id, const char* tname) noexcept;

    /**
     *  @see ::vsm_destroy_zone
     */
    VsmStatus vsm_destroy_zone(const char* id) noexcept;

    /**
     *  @see ::vsm_shutdown_zone
     */
    VsmStatus vsm_shutdown_zone(const char* id) noexcept;

    /**
     *  @see ::vsm_start_zone
     */
    VsmStatus vsm_start_zone(const char* id) noexcept;

    /**
     *  @see ::vsm_lock_zone
     */
    VsmStatus vsm_lock_zone(const char* id) noexcept;

    /**
     *  @see ::vsm_unlock_zone
     */
    VsmStatus vsm_unlock_zone(const char* id) noexcept;

    /**
     *  @see ::vsm_add_state_callback
     */
    VsmStatus vsm_add_state_callback(VsmZoneDbusStateCallback zoneDbusStateCallback,
                                     void* data,
                                     VsmSubscriptionId* subscriptionId) noexcept;

    /**
     *  @see ::vsm_del_state_callback
     */
    VsmStatus vsm_del_state_callback(VsmSubscriptionId subscriptionId) noexcept;

    /**
     *  @see ::vsm_del_state_callback
     */
    VsmStatus vsm_grant_device(const char* id,
                               const char* device,
                               uint32_t flags) noexcept;

    /**
     *  @see ::vsm_revoke_device
     */
    VsmStatus vsm_revoke_device(const char* id, const char* device) noexcept;

    /**
     *  @see ::vsm_zone_get_netdevs
     */
    VsmStatus vsm_zone_get_netdevs(const char* zone, VsmArrayString* netdevIds) noexcept;

    /**
     *  @see ::vsm_netdev_get_ipv4_addr
     */
    VsmStatus vsm_netdev_get_ipv4_addr(const char* zone,
                                       const char* netdevId,
                                       struct in_addr *addr) noexcept;

    /**
     *  @see ::vsm_netdev_get_ipv6_addr
     */
    VsmStatus vsm_netdev_get_ipv6_addr(const char* zone,
                                       const char* netdevId,
                                       struct in6_addr *addr) noexcept;

    /**
     *  @see ::vsm_netdev_set_ipv4_addr
     */
    VsmStatus vsm_netdev_set_ipv4_addr(const char* zone,
                                       const char* netdevId,
                                       struct in_addr *addr,
                                       int prefix) noexcept;

    /**
     *  @see ::vsm_netdev_set_ipv6_addr
     */
    VsmStatus vsm_netdev_set_ipv6_addr(const char* zone,
                                       const char* netdevId,
                                       struct in6_addr *addr,
                                       int prefix) noexcept;

    /**
     *  @see ::vsm_netdev_del_ipv4_addr
     */
    VsmStatus vsm_netdev_del_ipv4_addr(const char* zone,
                                      const char* netdevId,
                                      struct in_addr* addr,
                                      int prefix) noexcept;

    /**
     *  @see ::vsm_netdev_del_ipv6_addr
     */
    VsmStatus vsm_netdev_del_ipv6_addr(const char* zone,
                                      const char* netdevId,
                                      struct in6_addr* addr,
                                      int prefix) noexcept;

    /**
     *  @see ::vsm_netdev_up
     */
    VsmStatus vsm_netdev_up(const char* zone, const char* netdevId) noexcept;

    /**
     *  @see ::vsm_netdev_down
     */
    VsmStatus vsm_netdev_down(const char* zone, const char* netdevId) noexcept;

    /**
     *  @see ::vsm_create_netdev_veth
     */
    VsmStatus vsm_create_netdev_veth(const char* zone,
                                     const char* zoneDev,
                                     const char* hostDev) noexcept;

    /**
     *  @see ::vsm_create_netdev_macvlan
     */
    VsmStatus vsm_create_netdev_macvlan(const char* zone,
                                        const char* zoneDev,
                                        const char* hostDev,
                                        enum macvlan_mode mode) noexcept;

    /**
     *  @see ::vsm_create_netdev_phys
     */
    VsmStatus vsm_create_netdev_phys(const char* zone, const char* devId) noexcept;

    /**
     *  @see ::vsm_lookup_netdev_by_name
     */
    VsmStatus vsm_lookup_netdev_by_name(const char* zone,
                                        const char* netdevId,
                                        VsmNetdev* netdev) noexcept;

    /**
     *  @see ::vsm_destroy_netdev
     */
    VsmStatus vsm_destroy_netdev(const char* zone, const char* devId) noexcept;

    /**
     *  @see ::vsm_declare_file
     */
    VsmStatus vsm_declare_file(const char* zone,
                               VsmFileType type,
                               const char* path,
                               int32_t flags,
                               mode_t mode,
                               VsmString* id) noexcept;

    /**
     * @see ::vsm_declare_mount
     */
    VsmStatus vsm_declare_mount(const char* source,
                                const char* zone,
                                const char* target,
                                const char* type,
                                uint64_t flags,
                                const char* data,
                                VsmString* id) noexcept;
    /**
     * @see ::vsm_declare_link
     */
    VsmStatus vsm_declare_link(const char* source,
                               const char* zone,
                               const char* target,
                               VsmString* id) noexcept;

    /**
     * @see ::vsm_list_declarations
     */
    VsmStatus vsm_list_declarations(const char* zone, VsmArrayString* declarations) noexcept;

    /**
     * @see ::vsm_remove_declaration
     */
    VsmStatus vsm_remove_declaration(const char* zone, VsmString declaration) noexcept;

    /**
     *  @see ::vsm_notify_active_zone
     */
    VsmStatus vsm_notify_active_zone(const char* application, const char* message) noexcept;

    /**
     *  @see ::vsm_file_move_request
     */
    VsmStatus vsm_file_move_request(const char* destZone, const char* path) noexcept;

    /**
     *  @see ::vsm_add_notification_callback
     */
    VsmStatus vsm_add_notification_callback(VsmNotificationCallback notificationCallback,
                                            void* data,
                                            VsmSubscriptionId* subscriptionId) noexcept;

    /**
     *  @see ::vsm_del_notification_callback
     */
    VsmStatus vsm_del_notification_callback(VsmSubscriptionId subscriptionId) noexcept;

    /**
     *  @see ::vsm_start_glib_loop
     */
    static VsmStatus vsm_start_glib_loop() noexcept;

    /**
     *  @see ::vsm_stop_glib_loop
     */
    static VsmStatus vsm_stop_glib_loop() noexcept;
private:
#ifdef DBUS_CONNECTION
    typedef vasum::client::HostDbusConnection HostConnection;
#else
    typedef vasum::client::HostIPCConnection HostConnection;
#endif
    typedef vasum::client::ZoneDbusConnection ZoneConnection;
    struct Status {
        Status();
        Status(VsmStatus status, const std::string& msg = "");
        VsmStatus mVsmStatus;
        std::string mMsg;
    };

    Status mStatus;

    HostConnection mHostClient;
    ZoneConnection mZoneClient;

    VsmStatus coverException(const std::function<void(void)>& worker) noexcept;
    VsmStatus vsm_netdev_get_ip_addr(const char* zone,
                                     const char* netdevId,
                                     int type,
                                     void* addr) noexcept;
};

#endif /* VASUM_CLIENT_IMPL_HPP */
