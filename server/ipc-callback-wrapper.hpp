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
 * @brief   IPCSignalWrapper and IPCMethodWrapper classes used to hide IPC specifics
 */


#ifndef SERVER_IPC_CALLBACK_WRAPPER_HPP
#define SERVER_IPC_CALLBACK_WRAPPER_HPP

#include "api/messages.hpp"
#include "api/method-result-builder.hpp"
#include "api/ipc-method-result-builder.hpp"

#include <functional>

namespace vasum {

template<typename ArgIn>
class IPCSignalWrapper {
public:
    typedef typename std::remove_cv<ArgIn>::type in;
    typedef std::function<void(const in&)> type;

    static typename cargo::ipc::SignalHandler<in>::type
    getWrapper(const type& callback) {
        return [callback](const cargo::ipc::PeerID, const std::shared_ptr<in>& argIn)
        {
            callback(*argIn);
            return cargo::ipc::HandlerExitCode::SUCCESS;
        };
    }
};

template<>
class IPCSignalWrapper<const api::Void> {
public:
    typedef api::Void in;
    typedef std::function<void()> type;

    static typename cargo::ipc::SignalHandler<in>::type
    getWrapper(const type& callback) {
        return [callback](const cargo::ipc::PeerID, const std::shared_ptr<in>& /* argIn */)
        {
            callback();
            return cargo::ipc::HandlerExitCode::SUCCESS;
        };
    }
};

template<typename ArgIn = const api::Void, typename ArgOut = api::Void>
class IPCMethodWrapper {
public:
    typedef typename std::remove_cv<ArgIn>::type in;
    typedef ArgOut out;
    typedef std::function<void(const in&, api::MethodResultBuilder::Pointer)> type;

    static typename cargo::ipc::MethodHandler<out, in>::type
    getWrapper(const type& callback) {
        return [callback](const cargo::ipc::PeerID,
                          const std::shared_ptr<in>& argIn,
                          cargo::ipc::MethodResult::Pointer&& argOut)
        {
            auto rb = std::make_shared<api::IPCMethodResultBuilder>(argOut);
            callback(*argIn, rb);
            return cargo::ipc::HandlerExitCode::SUCCESS;
        };
    }
};

template<typename ArgOut>
class IPCMethodWrapper<ArgOut, typename std::enable_if<!std::is_const<ArgOut>::value, api::Void>::type> {
public:
    typedef api::Void in;
    typedef ArgOut out;
    typedef std::function<void(api::MethodResultBuilder::Pointer)> type;

    static typename cargo::ipc::MethodHandler<out, in>::type
    getWrapper(const type& callback) {
        return [callback](const cargo::ipc::PeerID,
                          const std::shared_ptr<in>& /* argIn */,
                          cargo::ipc::MethodResult::Pointer&& argOut)
        {
            auto rb = std::make_shared<api::IPCMethodResultBuilder>(argOut);
            callback(rb);
            return cargo::ipc::HandlerExitCode::SUCCESS;
        };
    }
};

} // namespace vasum

#endif // SERVER_IPC_CALLBACK_WRAPPER_HPP
