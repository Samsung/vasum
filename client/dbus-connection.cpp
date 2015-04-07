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
 * @brief   libSimpleDbus's wrapper
 */

#include <config.hpp>
#include "dbus-connection.hpp"
#include "exception.hpp"
#include <dbus/connection.hpp>
#include <gio/gio.h>

using namespace vasum::client;


DbusConnection::DbusConnection(const std::string& definition,
                               const std::string& busName,
                               const std::string& objectPath,
                               const std::string& interface)
    : mDefinition(definition)
    , mBusName(busName)
    , mObjectPath(objectPath)
    , mInterface(interface)
{
}

DbusConnection::~DbusConnection()
{
}

void DbusConnection::create(const std::shared_ptr<dbus::DbusConnection>& connection)
{
    mConnection = connection;
}

void DbusConnection::callMethod(const std::string& method,
                                GVariant* args_in,
                                const std::string& args_spec_out,
                                GVariant** args_out)
{
    dbus::GVariantPtr ret = mConnection->callMethod(mBusName,
                                                    mObjectPath,
                                                    mInterface,
                                                    method,
                                                    args_in,
                                                    args_spec_out);
    if (args_out != NULL) {
        *args_out = ret.release();
    }
}

DbusConnection::SubscriptionId DbusConnection::signalSubscribe(const std::string& signal,
                                                               const SignalCallback& signalCallback)
{
    auto onSignal = [this, signal, signalCallback](const std::string& /*senderBusName*/,
                                                   const std::string& objectPath,
                                                   const std::string& interface,
                                                   const std::string& signalName,
                                                   GVariant * parameters) {
        if (objectPath == mObjectPath &&
            interface == mInterface &&
            signalName == signal) {

            signalCallback(parameters);
        }
    };
    return mConnection->signalSubscribe(onSignal, mBusName);
}

void DbusConnection::signalUnsubscribe(SubscriptionId id)
{
    mConnection->signalUnsubscribe(id);
}

std::string DbusConnection::getArgsOutSpec(const std::string& methodName)
{
    //TODO: Information about output argumnets of all methods can be computed in constuctor
    GError *error = NULL;
    GDBusNodeInfo* nodeInfo = g_dbus_node_info_new_for_xml(mDefinition.c_str(), &error);
    if (error) {
        std::string msg = error->message;
        g_error_free (error);
        throw ClientException("Invalid xml: " + msg);
    }
    GDBusInterfaceInfo* interfaceInfo = g_dbus_node_info_lookup_interface(nodeInfo, mInterface.c_str());
    if (interfaceInfo == NULL) {
        throw ClientException("Invalid xml: can't find interface: " + mInterface);
    }
    GDBusMethodInfo* methodInfo = g_dbus_interface_info_lookup_method(interfaceInfo, methodName.c_str());
    if (methodInfo == NULL) {
        throw ClientException("Invalid xml: can't find method: " + methodName);
    }

    std::string signature;
    for (GDBusArgInfo** argInfo = methodInfo->out_args; *argInfo; ++argInfo) {
        signature += (*argInfo)->signature;
    }
    g_dbus_node_info_unref(nodeInfo);
    return "(" + signature + ")";
}
