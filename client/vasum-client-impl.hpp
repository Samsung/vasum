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
#include "host-ipc-connection.hpp"

#include <ipc/epoll/thread-dispatcher.hpp>
#include <ipc/epoll/event-poll.hpp>

#include <mutex>
#include <memory>
#include <functional>
#include <linux/if_link.h>

/**
 * Zone's D-Bus state change callback function signature.
 *
 * @param[in] zoneId affected zone id
 * @param[in] dbusAddress new D-Bus address
 * @param data custom user's data pointer passed to vsm_add_state_callback() function
 */
typedef std::function<void (const char *zoneId, const char *dbusAddress, void *data)> VsmZoneDbusStateFunction;

/**
 * Notification callback function signature.
 *
 * @param[in] zone source zone
 * @param[in] application sending application name
 * @param[in] message notification message
 * @param data custom user's data pointer passed to vsm_add_notification_callback()
 */
typedef std::function<void(const char *zone, const char *application, const char *message, void *data)>
    VsmNotificationFunction;

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
     * Connect client with system ipc address.
     *
     * @return status of this function call
     */
    VsmStatus connectSystem() noexcept;

    /**
     * Connect client.
     *
     * @param address ipc socket address
     * @return status of this function call
     */
    VsmStatus connect(const std::string& address) noexcept;

    /**
     * Disconnect client
     */
    VsmStatus disconnect() noexcept;

    /**
     *  @see ::vsm_get_poll_fd
     */
    VsmStatus vsm_get_poll_fd(int* fd) noexcept;

    /**
     *  @see ::vsm_enter_eventloop
     */
    VsmStatus vsm_enter_eventloop(int flags, int timeout) noexcept;

    /**
     *  @see ::vsm_set_dispatcher_type
     */
    VsmStatus vsm_set_dispatcher_type(VsmDispacherType dispacher) noexcept;

    /**
     *  @see ::vsm_get_dispatcher_type
     */
    VsmStatus vsm_get_dispatcher_type(VsmDispacherType* dispacher) noexcept;

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
     *  @see ::vsm_get_zone_rootpath
     */
    VsmStatus vsm_get_zone_rootpath(const char* id, VsmString* rootpath) noexcept;

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
    VsmStatus vsm_add_state_callback(VsmZoneDbusStateFunction zoneDbusStateCallback,
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
    VsmStatus vsm_add_notification_callback(VsmNotificationFunction notificationCallback,
                                            void* data,
                                            VsmSubscriptionId* subscriptionId) noexcept;

    /**
     *  @see ::vsm_del_notification_callback
     */
    VsmStatus vsm_del_notification_callback(VsmSubscriptionId subscriptionId) noexcept;

private:
    typedef vasum::client::HostIPCConnection HostConnection;
    struct Status {
        Status();
        Status(VsmStatus status, const std::string& msg = "");
        VsmStatus mVsmStatus;
        std::string mMsg;
    };
    Status mStatus;

    mutable std::mutex mStatusMutex;
    std::unique_ptr<ipc::epoll::ThreadDispatcher> mInternalDispatcher;
    std::unique_ptr<ipc::epoll::EventPoll> mEventPoll;
    HostConnection mHostClient;

    bool isInternalDispatcherEnabled() const;
    ipc::epoll::EventPoll& getEventPoll() const;
    VsmStatus coverException(const std::function<void(void)>& worker) noexcept;
    VsmStatus vsm_netdev_get_ip_addr(const char* zone,
                                     const char* netdevId,
                                     int type,
                                     void* addr) noexcept;
};

#endif /* VASUM_CLIENT_IMPL_HPP */
