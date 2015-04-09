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
 * @brief   IPCConnection class
 */

#ifndef VASUM_CLIENT_IPC_CONNECTION_HPP
#define VASUM_CLIENT_IPC_CONNECTION_HPP

#include <api/messages.hpp>
#include <ipc/client.hpp>
#include <ipc/types.hpp>
#include <epoll/thread-dispatcher.hpp>
#include <type_traits>
#include <memory>

namespace vasum {
namespace client {

/**
 * IPCConnection class
 */
class IPCConnection {
public:
    IPCConnection();
    virtual ~IPCConnection();

    void createSystem();

    template<typename ArgIn, typename ArgOut>
    typename std::enable_if<!std::is_integral<ArgOut>::value>::type
    call(const ipc::MethodID method, const ArgIn& argIn, ArgOut& argOut, unsigned int timeout = 50000) {
        auto out = mClient->callSync<ArgIn, ArgOut>(method, std::make_shared<ArgIn>(argIn), timeout);
        argOut = *out;
    }

    template<typename ArgOut>
    typename std::enable_if<!std::is_const<ArgOut>::value>::type
    call(const ipc::MethodID method, ArgOut& argOut, unsigned int timeout = 50000) {
        vasum::api::Void argIn;
        call(method, argIn, argOut, timeout);
    }

    template<typename ArgIn>
    typename std::enable_if<std::is_const<ArgIn>::value>::type
    call(const ipc::MethodID method, ArgIn& argIn, unsigned int timeout = 50000) {
        vasum::api::Void argOut;
        call(method, argIn, argOut, timeout);
    }

    template<typename Callback, typename ArgIn>
    void subscribe(const ipc::MethodID signal, const Callback& callback) {
        auto callbackWrapper = [callback] (const ipc::PeerID, std::shared_ptr<ArgIn>& data) {
            callback(*data);
        };
        mClient->setSignalHandler<ArgIn>(signal, callbackWrapper);
    }

    void unsubscribe(const ipc::MethodID signal) {
        mClient->removeMethod(signal);
    }

private:
    epoll::ThreadDispatcher mDispatcher;
    std::unique_ptr<ipc::Client> mClient;
};

} // namespace client
} // namespace vasum

#endif /* VASUM_CLIENT_IPC_CONNECTION_HPP */
