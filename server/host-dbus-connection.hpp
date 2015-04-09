/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Piotr Bartosiewicz <p.bartosiewi@partner.samsung.com>
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
 * @author  Piotr Bartosiewicz (p.bartosiewi@partner.samsung.com)
 * @brief   Declaration of a class for communication with server
 */


#ifndef SERVER_HOST_DBUS_CONNECTION_HPP
#define SERVER_HOST_DBUS_CONNECTION_HPP

#include "dbus/connection.hpp"
#include "api/method-result-builder.hpp"
#include "api/messages.hpp"

#include <functional>
#include <mutex>
#include <condition_variable>
#include <tuple>
#include <vector>


namespace vasum {


class HostDbusConnection {

public:
    HostDbusConnection();
    ~HostDbusConnection();

    // ------------- API --------------

    typedef std::function<void(const std::string& target,
                               const std::string& targetBusName,
                               const std::string& targetObjectPath,
                               const std::string& targetInterface,
                               const std::string& targetMethod,
                               GVariant* parameters,
                               dbus::MethodResultBuilder::Pointer result
                              )> ProxyCallCallback;
    typedef std::function<void(api::MethodResultBuilder::Pointer result
                              )> GetZoneDbusesCallback;
    typedef std::function<void(api::MethodResultBuilder::Pointer result
                              )> GetZoneIdsCallback;
    typedef std::function<void(api::MethodResultBuilder::Pointer result
                              )> GetActiveZoneIdCallback;
    typedef std::function<void(const api::ZoneId& dataIn,
                               api::MethodResultBuilder::Pointer result
                              )> GetZoneInfoCallback;
    typedef std::function<void(const api::SetNetDevAttrsIn& dataIn,
                               api::MethodResultBuilder::Pointer result
                              )> SetNetdevAttrsCallback;
    typedef std::function<void(const api::GetNetDevAttrsIn& dataIn,
                               api::MethodResultBuilder::Pointer result
                              )> GetNetdevAttrsCallback;
    typedef std::function<void(const api::ZoneId& dataIn,
                               api::MethodResultBuilder::Pointer result
                              )> GetNetdevListCallback;
    typedef std::function<void(const api::CreateNetDevVethIn& dataIn,
                               api::MethodResultBuilder::Pointer result
                              )> CreateNetdevVethCallback;
    typedef std::function<void(const api::CreateNetDevMacvlanIn& dataIn,
                               api::MethodResultBuilder::Pointer result
                              )> CreateNetdevMacvlanCallback;
    typedef std::function<void(const api::CreateNetDevPhysIn& dataIn,
                               api::MethodResultBuilder::Pointer result
                              )> CreateNetdevPhysCallback;
    typedef std::function<void(const api::DeleteNetdevIpAddressIn& dataIn,
                               api::MethodResultBuilder::Pointer result
                              )> DeleteNetdevIpAddressCallback;
    typedef std::function<void(const api::DestroyNetDevIn& dataIn,
                               api::MethodResultBuilder::Pointer result
                              )> DestroyNetdevCallback;
    typedef std::function<void(const api::DeclareFileIn& dataIn,
                               api::MethodResultBuilder::Pointer result
                              )> DeclareFileCallback;
    typedef std::function<void(const api::DeclareMountIn& dataIn,
                               api::MethodResultBuilder::Pointer result
                              )> DeclareMountCallback;
    typedef std::function<void(const api::DeclareLinkIn& dataIn,
                               api::MethodResultBuilder::Pointer result
                              )> DeclareLinkCallback;
    typedef std::function<void(const api::ZoneId& dataIn,
                               api::MethodResultBuilder::Pointer result
                              )> GetDeclarationsCallback;
    typedef std::function<void(const api::RemoveDeclarationIn& dataIn,
                               api::MethodResultBuilder::Pointer result
                              )> RemoveDeclarationCallback;
    typedef std::function<void(const api::ZoneId& dataIn,
                               api::MethodResultBuilder::Pointer result
                              )> SetActiveZoneCallback;
    typedef std::function<void(const api::CreateZoneIn& dataIn,
                               api::MethodResultBuilder::Pointer result
                              )> CreateZoneCallback;
    typedef std::function<void(const api::ZoneId& dataIn,
                               api::MethodResultBuilder::Pointer result
                              )> DestroyZoneCallback;
    typedef std::function<void(const api::ZoneId& dataIn,
                               api::MethodResultBuilder::Pointer result
                              )> ShutdownZoneCallback;
    typedef std::function<void(const api::ZoneId& dataIn,
                               api::MethodResultBuilder::Pointer result
                              )> StartZoneCallback;
    typedef std::function<void(const api::ZoneId& dataIn,
                               api::MethodResultBuilder::Pointer result
                              )> LockZoneCallback;
    typedef std::function<void(const api::ZoneId& dataIn,
                               api::MethodResultBuilder::Pointer result
                              )> UnlockZoneCallback;
    typedef std::function<void(const api::GrantDeviceIn& dataIn,
                               api::MethodResultBuilder::Pointer result
                              )> GrantDeviceCallback;
    typedef std::function<void(const api::RevokeDeviceIn& dataIn,
                               api::MethodResultBuilder::Pointer result
                              )> RevokeDeviceCallback;

    /**
     * Register proxy call callback
     */
    void setProxyCallCallback(const ProxyCallCallback& callback);

    /**
     * Register get zone dbuses callback
     */
    void setGetZoneDbusesCallback(const GetZoneDbusesCallback& callback);

    /**
     * Send signal describing dbus address state change
     */
    void signalZoneDbusState(const api::DbusState& state);

    /**
     * Register a callback called to get a list of zone ids
     */
    void setGetZoneIdsCallback(const GetZoneIdsCallback& callback);

    /**
     * Register a callback called to get the active zone id
     */
    void setGetActiveZoneIdCallback(const GetActiveZoneIdCallback& callback);

    /**
     * Register a callback called to get the zone informations
     */
    void setGetZoneInfoCallback(const GetZoneInfoCallback& callback);

    /**
     * Register a callback called to set network device attributes
     */
    void setSetNetdevAttrsCallback(const SetNetdevAttrsCallback& callback);

    /**
     * Register a callback called to get network device attributes
     */
    void setGetNetdevAttrsCallback(const GetNetdevAttrsCallback& callback);

    /**
     * Register a callback called to get network device list
     */
    void setGetNetdevListCallback(const GetNetdevListCallback& callback);

    /**
     * Register a callback called to create veth
     */
    void setCreateNetdevVethCallback(const CreateNetdevVethCallback& callback);

    /**
     * Register a callback called to create macvlan
     */
    void setCreateNetdevMacvlanCallback(const CreateNetdevMacvlanCallback& callback);

    /**
     * Register a callback called to create/move phys
     */
    void setCreateNetdevPhysCallback(const CreateNetdevPhysCallback& callback);

    /**
     * Register a callback called to destroy netdev
     */
    void setDestroyNetdevCallback(const DestroyNetdevCallback& callback);

    /**
     * Register a callback called to remove ip address from netdev
     */
    void setDeleteNetdevIpAddressCallback(const DeleteNetdevIpAddressCallback& callback);

    /**
     * Register a callback called to declare file
     */
    void setDeclareFileCallback(const DeclareFileCallback& callback);

    /**
     * Register a callback called to declare mount point
     */
    void setDeclareMountCallback(const DeclareMountCallback& callback);

    /**
     * Register a callback called to declare link
     */
    void setDeclareLinkCallback(const DeclareLinkCallback& callback);

    /**
     * Register a callback called to list declarations
     */
    void setGetDeclarationsCallback(const GetDeclarationsCallback& callback);

    /**
     * Register a callback called to remove declarartion
     */
    void setRemoveDeclarationCallback(const RemoveDeclarationCallback& callback);

    /**
     * Register a callback called to set the active zone
     */
    void setSetActiveZoneCallback(const SetActiveZoneCallback& callback);

    /**
     * Register a callback called to create new zone
     */
    void setCreateZoneCallback(const CreateZoneCallback& callback);

    /**
     * Register a callback called to destroy zone
     */
    void setDestroyZoneCallback(const DestroyZoneCallback& callback);

    /**
     * Register a callback called to shutdown zone
     */
    void setShutdownZoneCallback(const ShutdownZoneCallback& callback);

    /**
     * Register a callback called to start zone
     */
    void setStartZoneCallback(const StartZoneCallback& callback);

    /**
     * Register a callback called to lock zone
     */
    void setLockZoneCallback(const LockZoneCallback& callback);

    /**
     * Register a callback called to unlock zone
     */
    void setUnlockZoneCallback(const UnlockZoneCallback& callback);

    /**
     * Register a callback called to grant device
     */
    void setGrantDeviceCallback(const GrantDeviceCallback& callback);

    /**
     * Register a callback called to revoke device
     */
    void setRevokeDeviceCallback(const RevokeDeviceCallback& callback);

    /**
     * Make a proxy call
     */
    void proxyCallAsync(const std::string& busName,
                        const std::string& objectPath,
                        const std::string& interface,
                        const std::string& method,
                        GVariant* parameters,
                        const dbus::DbusConnection::AsyncMethodCallCallback& callback);

private:
    dbus::DbusConnection::Pointer mDbusConnection;
    std::mutex mNameMutex;
    std::condition_variable mNameCondition;
    bool mNameAcquired;
    bool mNameLost;
    ProxyCallCallback mProxyCallCallback;
    GetZoneDbusesCallback mGetZoneDbusesCallback;
    GetZoneIdsCallback mGetZoneIdsCallback;
    GetActiveZoneIdCallback mGetActiveZoneIdCallback;
    GetZoneInfoCallback mGetZoneInfoCallback;
    SetNetdevAttrsCallback mSetNetdevAttrsCallback;
    GetNetdevAttrsCallback mGetNetdevAttrsCallback;
    GetNetdevListCallback mGetNetdevListCallback;
    CreateNetdevVethCallback mCreateNetdevVethCallback;
    CreateNetdevMacvlanCallback mCreateNetdevMacvlanCallback;
    CreateNetdevPhysCallback mCreateNetdevPhysCallback;
    DestroyNetdevCallback mDestroyNetdevCallback;
    DeleteNetdevIpAddressCallback mDeleteNetdevIpAddressCallback;
    DeclareFileCallback mDeclareFileCallback;
    DeclareMountCallback mDeclareMountCallback;
    DeclareLinkCallback mDeclareLinkCallback;
    GetDeclarationsCallback mGetDeclarationsCallback;
    RemoveDeclarationCallback mRemoveDeclarationCallback;
    SetActiveZoneCallback mSetActiveZoneCallback;
    CreateZoneCallback mCreateZoneCallback;
    DestroyZoneCallback mDestroyZoneCallback;
    ShutdownZoneCallback mShutdownZoneCallback;
    StartZoneCallback mStartZoneCallback;
    LockZoneCallback mLockZoneCallback;
    UnlockZoneCallback mUnlockZoneCallback;
    GrantDeviceCallback mGrantDeviceCallback;
    RevokeDeviceCallback mRevokeDeviceCallback;

    void onNameAcquired();
    void onNameLost();
    bool waitForName(const unsigned int timeoutMs);

    void onMessageCall(const std::string& objectPath,
                       const std::string& interface,
                       const std::string& methodName,
                       GVariant* parameters,
                       dbus::MethodResultBuilder::Pointer result);
};


} // namespace vasum


#endif // SERVER_HOST_DBUS_CONNECTION_HPP
