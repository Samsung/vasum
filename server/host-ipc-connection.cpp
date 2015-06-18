/*
 *  Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
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
 * @brief   HostIPCConnection class
 */

#include "config.hpp"

#include <functional>

#include "host-ipc-connection.hpp"
#include "host-ipc-definitions.hpp"
#include "exception.hpp"
#include "logger/logger.hpp"
#include "zones-manager.hpp"


namespace vasum {

HostIPCConnection::HostIPCConnection(ZonesManager* zonesManagerPtr)
    : mZonesManagerPtr(zonesManagerPtr)
{
    LOGT("Connecting to host IPC socket");

    ipc::PeerCallback removedCallback = [this](const ipc::PeerID peerID,
                                               const ipc::FileDescriptor) {
        std::string id = api::IPC_CONNECTION_PREFIX + std::to_string(peerID);
        mZonesManagerPtr->disconnectedCallback(id);
    };
    mService.reset(new ipc::Service(mDispatcher.getPoll(), HOST_IPC_SOCKET,
                                    nullptr, removedCallback));

    using namespace std::placeholders;
    setLockQueueCallback(std::bind(&ZonesManager::handleLockQueueCall,
                                   mZonesManagerPtr, _1));

    setUnlockQueueCallback(std::bind(&ZonesManager::handleUnlockQueueCall,
                                     mZonesManagerPtr, _1));

    setGetZoneIdsCallback(std::bind(&ZonesManager::handleGetZoneIdsCall,
                                    mZonesManagerPtr, _1));

    setGetActiveZoneIdCallback(std::bind(&ZonesManager::handleGetActiveZoneIdCall,
                                         mZonesManagerPtr, _1));

    setGetZoneInfoCallback(std::bind(&ZonesManager::handleGetZoneInfoCall,
                                     mZonesManagerPtr, _1, _2));

    setSetNetdevAttrsCallback(std::bind(&ZonesManager::handleSetNetdevAttrsCall,
                                        mZonesManagerPtr, _1, _2));

    setGetNetdevAttrsCallback(std::bind(&ZonesManager::handleGetNetdevAttrsCall,
                                        mZonesManagerPtr, _1, _2));

    setGetNetdevListCallback(std::bind(&ZonesManager::handleGetNetdevListCall,
                                       mZonesManagerPtr, _1, _2));

    setCreateNetdevVethCallback(std::bind(&ZonesManager::handleCreateNetdevVethCall,
                                          mZonesManagerPtr, _1, _2));

    setCreateNetdevMacvlanCallback(std::bind(&ZonesManager::handleCreateNetdevMacvlanCall,
                                             mZonesManagerPtr, _1, _2));

    setCreateNetdevPhysCallback(std::bind(&ZonesManager::handleCreateNetdevPhysCall,
                                          mZonesManagerPtr, _1, _2));

    setDestroyNetdevCallback(std::bind(&ZonesManager::handleDestroyNetdevCall,
                                       mZonesManagerPtr, _1, _2));

    setDeleteNetdevIpAddressCallback(std::bind(&ZonesManager::handleDeleteNetdevIpAddressCall,
                                               mZonesManagerPtr, _1, _2));

    setDeclareFileCallback(std::bind(&ZonesManager::handleDeclareFileCall,
                                     mZonesManagerPtr, _1, _2));

    setDeclareMountCallback(std::bind(&ZonesManager::handleDeclareMountCall,
                                      mZonesManagerPtr, _1, _2));

    setDeclareLinkCallback(std::bind(&ZonesManager::handleDeclareLinkCall,
                                     mZonesManagerPtr, _1, _2));

    setGetDeclarationsCallback(std::bind(&ZonesManager::handleGetDeclarationsCall,
                                         mZonesManagerPtr, _1, _2));

    setRemoveDeclarationCallback(std::bind(&ZonesManager::handleRemoveDeclarationCall,
                                           mZonesManagerPtr, _1, _2));

    setSetActiveZoneCallback(std::bind(&ZonesManager::handleSetActiveZoneCall,
                                       mZonesManagerPtr, _1, _2));

    setCreateZoneCallback(std::bind(&ZonesManager::handleCreateZoneCall,
                                    mZonesManagerPtr, _1, _2));

    setDestroyZoneCallback(std::bind(&ZonesManager::handleDestroyZoneCall,
                                     mZonesManagerPtr, _1, _2));

    setShutdownZoneCallback(std::bind(&ZonesManager::handleShutdownZoneCall,
                                      mZonesManagerPtr, _1, _2));

    setStartZoneCallback(std::bind(&ZonesManager::handleStartZoneCall,
                                   mZonesManagerPtr, _1, _2));

    setLockZoneCallback(std::bind(&ZonesManager::handleLockZoneCall,
                                  mZonesManagerPtr, _1, _2));

    setUnlockZoneCallback(std::bind(&ZonesManager::handleUnlockZoneCall,
                                    mZonesManagerPtr, _1, _2));

    setGrantDeviceCallback(std::bind(&ZonesManager::handleGrantDeviceCall,
                                     mZonesManagerPtr, _1, _2));

    setRevokeDeviceCallback(std::bind(&ZonesManager::handleRevokeDeviceCall,
                                      mZonesManagerPtr, _1, _2));

    setNotifyActiveZoneCallback(std::bind(&ZonesManager::handleNotifyActiveZoneCall,
                                          mZonesManagerPtr, "", _1, _2));

    setSwitchToDefaultCallback(std::bind(&ZonesManager::handleSwitchToDefaultCall,
                                         mZonesManagerPtr, "", _1));

    setFileMoveCallback(std::bind(&ZonesManager::handleFileMoveCall,
                                  mZonesManagerPtr, "", _1, _2));

    setCreateFileCallback(std::bind(&ZonesManager::handleCreateFileCall,
                                    mZonesManagerPtr, _1, _2));
}

HostIPCConnection::~HostIPCConnection()
{
}

void HostIPCConnection::start()
{
    LOGT("Starting IPC");
    mService->start();
    LOGD("Connected");
}

void HostIPCConnection::setLockQueueCallback(const Method<api::Void>::type& callback)
{
    typedef IPCMethodWrapper<api::Void> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::ipc::METHOD_LOCK_QUEUE,
        Callback::getWrapper(callback));
}

void HostIPCConnection::setUnlockQueueCallback(const Method<api::Void>::type& callback)
{
    typedef IPCMethodWrapper<api::Void> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::ipc::METHOD_UNLOCK_QUEUE,
        Callback::getWrapper(callback));
}

void HostIPCConnection::setGetZoneIdsCallback(const Method<api::ZoneIds>::type& callback)
{
    typedef IPCMethodWrapper<api::ZoneIds> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::ipc::METHOD_GET_ZONE_ID_LIST,
        Callback::getWrapper(callback));
}

void HostIPCConnection::setGetActiveZoneIdCallback(const Method<api::ZoneId>::type& callback)
{
    typedef IPCMethodWrapper<api::ZoneId> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::ipc::METHOD_GET_ACTIVE_ZONE_ID,
        Callback::getWrapper(callback));
}

void HostIPCConnection::setGetZoneInfoCallback(const Method<const api::ZoneId, api::ZoneInfoOut>::type& callback)
{
    typedef IPCMethodWrapper<const api::ZoneId, api::ZoneInfoOut> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::ipc::METHOD_GET_ZONE_INFO,
        Callback::getWrapper(callback));
}

void HostIPCConnection::setSetNetdevAttrsCallback(const Method<const api::SetNetDevAttrsIn>::type& callback)
{
    typedef IPCMethodWrapper<const api::SetNetDevAttrsIn> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::ipc::METHOD_SET_NETDEV_ATTRS,
        Callback::getWrapper(callback));
}

void HostIPCConnection::setGetNetdevAttrsCallback(const Method<const api::GetNetDevAttrsIn, api::GetNetDevAttrs>::type& callback)
{
    typedef IPCMethodWrapper<const api::GetNetDevAttrsIn, api::GetNetDevAttrs> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::ipc::METHOD_GET_NETDEV_ATTRS,
        Callback::getWrapper(callback));
}

void HostIPCConnection::setGetNetdevListCallback(const Method<const api::ZoneId, api::NetDevList>::type& callback)
{
    typedef IPCMethodWrapper<const api::ZoneId, api::NetDevList> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::ipc::METHOD_GET_NETDEV_LIST,
        Callback::getWrapper(callback));
}

void HostIPCConnection::setCreateNetdevVethCallback(const Method<const api::CreateNetDevVethIn>::type& callback)
{
    typedef IPCMethodWrapper<const api::CreateNetDevVethIn> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::ipc::METHOD_CREATE_NETDEV_VETH,
        Callback::getWrapper(callback));
}

void HostIPCConnection::setCreateNetdevMacvlanCallback(const Method<const api::CreateNetDevMacvlanIn>::type& callback)
{
    typedef IPCMethodWrapper<const api::CreateNetDevMacvlanIn> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::ipc::METHOD_CREATE_NETDEV_MACVLAN,
        Callback::getWrapper(callback));
}

void HostIPCConnection::setCreateNetdevPhysCallback(const Method<const api::CreateNetDevPhysIn>::type& callback)
{
    typedef IPCMethodWrapper<const api::CreateNetDevPhysIn> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::ipc::METHOD_CREATE_NETDEV_PHYS,
        Callback::getWrapper(callback));
}

void HostIPCConnection::setDestroyNetdevCallback(const Method<const api::DestroyNetDevIn>::type& callback)
{
    typedef IPCMethodWrapper<const api::DestroyNetDevIn> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::ipc::METHOD_DESTROY_NETDEV,
        Callback::getWrapper(callback));
}

void HostIPCConnection::setDeleteNetdevIpAddressCallback(const Method<const api::DeleteNetdevIpAddressIn>::type& callback)
{
    typedef IPCMethodWrapper<const api::DeleteNetdevIpAddressIn> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::ipc::METHOD_DELETE_NETDEV_IP_ADDRESS,
        Callback::getWrapper(callback));
}

void HostIPCConnection::setDeclareFileCallback(const Method<const api::DeclareFileIn, api::Declaration>::type& callback)
{
    typedef IPCMethodWrapper<const api::DeclareFileIn, api::Declaration> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::ipc::METHOD_DECLARE_FILE,
        Callback::getWrapper(callback));
}

void HostIPCConnection::setDeclareMountCallback(const Method<const api::DeclareMountIn, api::Declaration>::type& callback)
{
    typedef IPCMethodWrapper<const api::DeclareMountIn, api::Declaration> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::ipc::METHOD_DECLARE_MOUNT,
        Callback::getWrapper(callback));
}

void HostIPCConnection::setDeclareLinkCallback(const Method<const api::DeclareLinkIn, api::Declaration>::type& callback)
{
    typedef IPCMethodWrapper<const api::DeclareLinkIn, api::Declaration> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::ipc::METHOD_DECLARE_LINK,
        Callback::getWrapper(callback));
}

void HostIPCConnection::setGetDeclarationsCallback(const Method<const api::ZoneId, api::Declarations>::type& callback)
{
    typedef IPCMethodWrapper<const api::ZoneId, api::Declarations> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::ipc::METHOD_GET_DECLARATIONS,
        Callback::getWrapper(callback));
}

void HostIPCConnection::setRemoveDeclarationCallback(const Method<const api::RemoveDeclarationIn>::type& callback)
{
    typedef IPCMethodWrapper<const api::RemoveDeclarationIn> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::ipc::METHOD_REMOVE_DECLARATION,
        Callback::getWrapper(callback));
}

void HostIPCConnection::setSetActiveZoneCallback(const Method<const api::ZoneId>::type& callback)
{
    typedef IPCMethodWrapper<const api::ZoneId> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::ipc::METHOD_SET_ACTIVE_ZONE,
        Callback::getWrapper(callback));
}

void HostIPCConnection::setCreateZoneCallback(const Method<const api::CreateZoneIn>::type& callback)
{
    typedef IPCMethodWrapper<const api::CreateZoneIn> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::ipc::METHOD_CREATE_ZONE,
        Callback::getWrapper(callback));
}

void HostIPCConnection::setDestroyZoneCallback(const Method<const api::ZoneId>::type& callback)
{
    typedef IPCMethodWrapper<const api::ZoneId> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::ipc::METHOD_DESTROY_ZONE,
        Callback::getWrapper(callback));
}

void HostIPCConnection::setShutdownZoneCallback(const Method<const api::ZoneId>::type& callback)
{
    typedef IPCMethodWrapper<const api::ZoneId> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::ipc::METHOD_SHUTDOWN_ZONE,
        Callback::getWrapper(callback));
}

void HostIPCConnection::setStartZoneCallback(const Method<const api::ZoneId>::type& callback)
{
    typedef IPCMethodWrapper<const api::ZoneId> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::ipc::METHOD_START_ZONE,
        Callback::getWrapper(callback));
}

void HostIPCConnection::setLockZoneCallback(const Method<const api::ZoneId>::type& callback)
{
    typedef IPCMethodWrapper<const api::ZoneId> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::ipc::METHOD_LOCK_ZONE,
        Callback::getWrapper(callback));
}

void HostIPCConnection::setUnlockZoneCallback(const Method<const api::ZoneId>::type& callback)
{
    typedef IPCMethodWrapper<const api::ZoneId> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::ipc::METHOD_UNLOCK_ZONE,
        Callback::getWrapper(callback));
}

void HostIPCConnection::setGrantDeviceCallback(const Method<const api::GrantDeviceIn>::type& callback)
{
    typedef IPCMethodWrapper<const api::GrantDeviceIn> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::ipc::METHOD_GRANT_DEVICE,
        Callback::getWrapper(callback));
}

void HostIPCConnection::setRevokeDeviceCallback(const Method<const api::RevokeDeviceIn>::type& callback)
{
    typedef IPCMethodWrapper<const api::RevokeDeviceIn> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::ipc::METHOD_REVOKE_DEVICE,
        Callback::getWrapper(callback));
}

void HostIPCConnection::setNotifyActiveZoneCallback(
    const Method<const vasum::api::NotifActiveZoneIn>::type& callback)
{
    typedef IPCMethodWrapper<const api::NotifActiveZoneIn> Method;
    mService->setMethodHandler<Method::out, Method::in>(
        api::ipc::METHOD_NOTIFY_ACTIVE_ZONE,
        Method::getWrapper(callback));
}

void HostIPCConnection::setSwitchToDefaultCallback(const Method<api::Void>::type& callback)
{
    typedef IPCMethodWrapper<api::Void> Callback;
    mService->setMethodHandler<Callback::out, Callback::in>(
        api::ipc::METHOD_SWITCH_TO_DEFAULT,
        Callback::getWrapper(callback));
}

void HostIPCConnection::setFileMoveCallback(const Method<const api::FileMoveRequestIn,
                                            api::FileMoveRequestStatus>::type& callback)
{
    typedef IPCMethodWrapper<const api::FileMoveRequestIn, api::FileMoveRequestStatus> Method;
    mService->setMethodHandler<Method::out, Method::in>(
        api::ipc::METHOD_FILE_MOVE_REQUEST,
        Method::getWrapper(callback));
}

void HostIPCConnection::setCreateFileCallback(const Method<const api::CreateFileIn,
                                              api::CreateFileOut>::type& callback)
{
    typedef IPCMethodWrapper<const api::CreateFileIn, api::CreateFileOut> Method;
    mService->setMethodHandler<Method::out, Method::in>(
        api::ipc::METHOD_CREATE_FILE,
        Method::getWrapper(callback));
}

void HostIPCConnection::sendNotification(const api::Notification& notification)
{
    mService->signal(api::ipc::SIGNAL_NOTIFICATION,
                     std::make_shared<api::Notification>(notification));
}

} // namespace vasum
