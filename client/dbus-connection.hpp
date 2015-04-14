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
 * @brief   SimpleDbus's wrapper
 */

#ifndef VASUM_CLIENT_DBUS_CONNECTION_HPP
#define VASUM_CLIENT_DBUS_CONNECTION_HPP

#include <api/messages.hpp>
#include <config/manager.hpp>
#include <dbus/connection.hpp>
#include <type_traits>
#include <string>
#include <memory>

namespace vasum {
namespace client {

/**
 * SimpleDbus client definition.
 *
 * DbusConnection uses SimpleDbus API.
 */
class DbusConnection {
public:
    typedef unsigned int SubscriptionId;

    DbusConnection(const std::string& definition,
                   const std::string& busName,
                   const std::string& objectPath,
                   const std::string& interface);
    virtual ~DbusConnection();
    void create(const std::shared_ptr<dbus::DbusConnection>& connection);

    template<typename ArgIn, typename ArgOut>
    typename std::enable_if<!std::is_same<ArgOut, vasum::api::Void>::value>::type
    call(const std::string& method, const ArgIn& argIn, ArgOut& argOut);

    template<typename ArgIn, typename ArgOut>
    typename std::enable_if<std::is_same<ArgOut, vasum::api::Void>::value>::type
    call(const std::string& method, const ArgIn& argIn, ArgOut& argOut);

    template<typename ArgOut>
    typename std::enable_if<!std::is_const<ArgOut>::value>::type
    call(const std::string& method, ArgOut& argOut) {
        vasum::api::Void argIn;
        call(method, argIn, argOut);
    }

    template<typename ArgIn>
    typename std::enable_if<std::is_const<ArgIn>::value>::type
    call(const std::string& method, ArgIn& argIn) {
        vasum::api::Void argOut;
        call(method, argIn, argOut);
    }

    template<typename Arg>
    SubscriptionId signalSubscribe(const std::string& signal,
                                   const std::function<void(const Arg& arg)>& signalCallback);
    void signalUnsubscribe(SubscriptionId id);

private:
    typedef std::function<void(GVariant* parameters)> SignalCallback;

    std::shared_ptr<dbus::DbusConnection> mConnection;
    const std::string mDefinition;
    const std::string mBusName;
    const std::string mObjectPath;
    const std::string mInterface;

    void callMethod(const std::string& method,
                    GVariant* args_in,
                    const std::string& args_spec_out,
                    GVariant** args_out);
    SubscriptionId signalSubscribe(const std::string& signal, const SignalCallback& signalCallback);

    /**
     * Get signature of method output parameters
     */
    std::string getArgsOutSpec(const std::string& methodName);
};

template<typename ArgIn, typename ArgOut>
typename std::enable_if<!std::is_same<ArgOut, vasum::api::Void>::value>::type
DbusConnection::call(const std::string& method, const ArgIn& argIn, ArgOut& argOut)
{
    GVariant* gArgOut = NULL;
    callMethod(method, config::saveToGVariant(argIn), getArgsOutSpec(method), &gArgOut);
    config::loadFromGVariant(gArgOut, argOut);
    g_variant_unref(gArgOut);
}

template<typename ArgIn, typename ArgOut>
typename std::enable_if<std::is_same<ArgOut, vasum::api::Void>::value>::type
DbusConnection::call(const std::string& method, const ArgIn& argIn, ArgOut& /* argOut */)
{
    callMethod(method, config::saveToGVariant(argIn), "", NULL);
}

template<typename Arg>
DbusConnection::SubscriptionId DbusConnection::signalSubscribe(const std::string& signal,
                                                               const std::function<void(const Arg& arg)>& signalCallback)
{
    SignalCallback callback = [signalCallback](GVariant* parameters) {
        Arg param;
        config::loadFromGVariant(parameters, param);
        signalCallback(param);
    };
    return signalSubscribe(signal, callback);
}

} // namespace client
} // namespace vasum

#endif /* VASUM_CLIENT_DBUS_CONNECTION_HPP */
