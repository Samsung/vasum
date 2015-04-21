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


#ifndef SERVER_HOST_IPC_CONNECTION_HPP
#define SERVER_HOST_IPC_CONNECTION_HPP

#include "api/messages.hpp"
#include "api/method-result-builder.hpp"
#include "api/ipc-method-result-builder.hpp"
#include "epoll/thread-dispatcher.hpp"
#include "ipc/service.hpp"

#include <functional>

namespace vasum {



class HostIPCConnection {
public:
    template<typename ArgIn, typename ArgOut = api::Void>
    class Callback {
    public:
        typedef typename std::remove_cv<ArgIn>::type in;
        typedef ArgOut out;
        typedef std::function<void(const in&, api::MethodResultBuilder::Pointer)> type;

        static typename ipc::MethodHandler<out, in>::type
        getCallbackWrapper(const type& callback) {
            return [callback](const ipc::PeerID,
                              const std::shared_ptr<in>& argIn,
                              ipc::MethodResult::Pointer&& argOut)
            {
                auto rb = std::make_shared<api::IPCMethodResultBuilder>(argOut);
                callback(*argIn, rb);
            };
        }
    };

    template<typename ArgOut>
    class Callback<ArgOut, typename std::enable_if<!std::is_const<ArgOut>::value, api::Void>::type> {
    public:
        typedef api::Void in;
        typedef ArgOut out;
        typedef std::function<void(api::MethodResultBuilder::Pointer)> type;

        static typename ipc::MethodHandler<out, in>::type
        getCallbackWrapper(const type& callback) {
            return [callback](const ipc::PeerID,
                              const std::shared_ptr<in>& /* argIn */,
                              ipc::MethodResult::Pointer&& argOut)
            {
                auto rb = std::make_shared<api::IPCMethodResultBuilder>(argOut);
                callback(rb);
            };
        }
    };

    HostIPCConnection();
    ~HostIPCConnection();

    void setGetZoneConnectionsCallback(const Callback<api::Connections>::type& callback);
    void setGetZoneIdsCallback(const Callback<api::ZoneIds>::type& callback);
    void setGetActiveZoneIdCallback(const Callback<api::ZoneId>::type& callback);
    void setGetZoneInfoCallback(const Callback<const api::ZoneId, api::ZoneInfoOut>::type& callback);
    void setSetNetdevAttrsCallback(const Callback<const api::SetNetDevAttrsIn>::type& callback);
    void setGetNetdevAttrsCallback(const Callback<const api::GetNetDevAttrsIn, api::GetNetDevAttrs>::type& callback);
    void setGetNetdevListCallback(const Callback<const api::ZoneId, api::NetDevList>::type& callback);
    void setCreateNetdevVethCallback(const Callback<const api::CreateNetDevVethIn>::type& callback);
    void setCreateNetdevMacvlanCallback(const Callback<const api::CreateNetDevMacvlanIn>::type& callback);
    void setCreateNetdevPhysCallback(const Callback<const api::CreateNetDevPhysIn>::type& callback);
    void setDestroyNetdevCallback(const Callback<const api::DestroyNetDevIn>::type& callback);
    void setDeleteNetdevIpAddressCallback(const Callback<const api::DeleteNetdevIpAddressIn>::type& callback);
    void setDeclareFileCallback(const Callback<const api::DeclareFileIn, api::Declaration>::type& callback);
    void setDeclareMountCallback(const Callback<const api::DeclareMountIn, api::Declaration>::type& callback);
    void setDeclareLinkCallback(const Callback<const api::DeclareLinkIn, api::Declaration>::type& callback);
    void setGetDeclarationsCallback(const Callback<const api::ZoneId, api::Declarations>::type& callback);
    void setRemoveDeclarationCallback(const Callback<const api::RemoveDeclarationIn>::type& callback);
    void setSetActiveZoneCallback(const Callback<const api::ZoneId>::type& callback);
    void setCreateZoneCallback(const Callback<const api::CreateZoneIn>::type& callback);
    void setDestroyZoneCallback(const Callback<const api::ZoneId>::type& callback);
    void setShutdownZoneCallback(const Callback<const api::ZoneId>::type& callback);
    void setStartZoneCallback(const Callback<const api::ZoneId>::type& callback);
    void setLockZoneCallback(const Callback<const api::ZoneId>::type& callback);
    void setUnlockZoneCallback(const Callback<const api::ZoneId>::type& callback);
    void setGrantDeviceCallback(const Callback<const api::GrantDeviceIn>::type& callback);
    void setRevokeDeviceCallback(const Callback<const api::RevokeDeviceIn>::type& callback);
    void signalZoneConnectionState(const api::ConnectionState& connectionState);

private:
    epoll::ThreadDispatcher mDispatcher;
    std::unique_ptr<ipc::Service> mService;
};

} // namespace vasum

#endif // SERVER_HOST_IPC_CONNECTION_HPP
