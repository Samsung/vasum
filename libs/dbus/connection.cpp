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
 * @brief   Dbus connection class
 */

#include "dbus/config.hpp"
#include "dbus/connection.hpp"
#include "dbus/exception.hpp"
#include "utils/callback-wrapper.hpp"
#include "utils/scoped-gerror.hpp"
#include "utils/glib-utils.hpp"
#include "logger/logger.hpp"

using namespace vasum::utils;

namespace dbus {


namespace {

const std::string SYSTEM_BUS_ADDRESS = "unix:path=/var/run/dbus/system_bus_socket";
const std::string INTROSPECT_INTERFACE = "org.freedesktop.DBus.Introspectable";
const std::string INTROSPECT_METHOD = "Introspect";

class MethodResultBuilderImpl : public MethodResultBuilder {
public:
    MethodResultBuilderImpl(GDBusMethodInvocation* invocation)
        : mInvocation(invocation), mResultSet(false) {}
    ~MethodResultBuilderImpl()
    {
        if (!mResultSet) {
            setError("org.freedesktop.DBus.Error.UnknownMethod", "Not implemented");
        }
    }
    void set(GVariant* parameters)
    {
        g_dbus_method_invocation_return_value(mInvocation, parameters);
        mResultSet = true;
    }
    void setVoid()
    {
        set(NULL);
    }
    void setError(const std::string& name, const std::string& message)
    {
        g_dbus_method_invocation_return_dbus_error(mInvocation, name.c_str(), message.c_str());
        mResultSet = true;
    }
private:
    GDBusMethodInvocation* mInvocation;
    bool mResultSet;
};

void throwDbusException(const ScopedGError& e)
{
    if (e->domain == g_io_error_quark()) {
        if (e->code == G_IO_ERROR_DBUS_ERROR) {
            throw DbusCustomException(e->message);
        } else {
            throw DbusIOException(e->message);
        }
    } else if (e->domain == g_dbus_error_quark()) {
        throw DbusOperationException(e->message);
    } else if (e->domain == g_markup_error_quark()) {
        throw DbusInvalidArgumentException(e->message);
    } else {
        throw DbusException(e->message);
    }
}

class AsyncMethodCallResultImpl : public AsyncMethodCallResult {
public:
    AsyncMethodCallResultImpl(GVariant* result, const ScopedGError& error)
        : mResult(result), mError(error) {}
    ~AsyncMethodCallResultImpl()
    {
        if (mResult) {
            g_variant_unref(mResult);
        }
    }
    GVariant* get()
    {
        if (mError) {
            throwDbusException(mError);
        }
        return mResult;
    }
private:
    GVariant* mResult;
    const ScopedGError& mError;
};

} // namespace

DbusConnection::Pointer DbusConnection::create(const std::string& address)
{
    return Pointer(new DbusConnection(address));
}

DbusConnection::Pointer DbusConnection::createSystem()
{
    return create(SYSTEM_BUS_ADDRESS);
}

DbusConnection::DbusConnection(const std::string& address)
    : mConnection(NULL)
    , mNameId(0)
{
    ScopedGError error;
    const GDBusConnectionFlags flags =
        static_cast<GDBusConnectionFlags>(G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT |
                                          G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION);
    // TODO: this is possible deadlock if the dbus
    // socket exists but there is no dbus-daemon
    mConnection = g_dbus_connection_new_for_address_sync(address.c_str(),
                                                         flags,
                                                         NULL,
                                                         NULL,
                                                         &error);
    if (error) {
        error.strip();
        LOGE("Could not connect to " << address << "; " << error);
        throwDbusException(error);
    }
}

DbusConnection::~DbusConnection()
{
    // Close connection in a glib thread (because of a bug in glib)
    GDBusConnection* connection = mConnection;
    guint nameId = mNameId;

    auto closeConnection = [=]() {
        if (nameId) {
            g_bus_unown_name(nameId);
        }
        g_object_unref(connection);
        LOGT("Connection deleted");
    };
    executeInGlibThread(closeConnection, mGuard);
}

void DbusConnection::setName(const std::string& name,
                             const VoidCallback& onNameAcquired,
                             const VoidCallback& onNameLost)
{
    mNameId = g_bus_own_name_on_connection(mConnection,
                                           name.c_str(),
                                           G_BUS_NAME_OWNER_FLAGS_NONE,
                                           &DbusConnection::onNameAcquired,
                                           &DbusConnection::onNameLost,
                                           createCallbackWrapper(
                                               NameCallbacks(onNameAcquired, onNameLost),
                                               mGuard.spawn()),
                                           &deleteCallbackWrapper<NameCallbacks>);
}

void DbusConnection::onNameAcquired(GDBusConnection*, const gchar* name, gpointer userData)
{
    LOGD("Name acquired " << name);
    const NameCallbacks& callbacks = getCallbackFromPointer<NameCallbacks>(userData);
    if (callbacks.nameAcquired) {
        callbacks.nameAcquired();
    }
}

void DbusConnection::onNameLost(GDBusConnection*, const gchar* name, gpointer userData)
{
    LOGD("Name lost " << name);
    const NameCallbacks& callbacks = getCallbackFromPointer<NameCallbacks>(userData);
    if (callbacks.nameLost) {
        callbacks.nameLost();
    }
}

void DbusConnection::emitSignal(const std::string& objectPath,
                                const std::string& interface,
                                const std::string& name,
                                GVariant* parameters)
{
    ScopedGError error;
    g_dbus_connection_emit_signal(mConnection,
                                  NULL,
                                  objectPath.c_str(),
                                  interface.c_str(),
                                  name.c_str(),
                                  parameters,
                                  &error);
    if (error) {
        error.strip();
        LOGE("Emit signal failed; " << error);
        throwDbusException(error);
    }
}

DbusConnection::SubscriptionId DbusConnection::signalSubscribe(const SignalCallback& callback,
                                                               const std::string& senderBusName)
{
    return g_dbus_connection_signal_subscribe(mConnection,
                                              senderBusName.empty() ? NULL : senderBusName.c_str(),
                                              NULL,
                                              NULL,
                                              NULL,
                                              NULL,
                                              G_DBUS_SIGNAL_FLAGS_NONE,
                                              &DbusConnection::onSignal,
                                              createCallbackWrapper(callback,
                                                                                mGuard.spawn()),
                                              &deleteCallbackWrapper<SignalCallback>);
}

void DbusConnection::signalUnsubscribe(DbusConnection::SubscriptionId subscriptionId)
{
    g_dbus_connection_signal_unsubscribe(mConnection, subscriptionId);
}

void DbusConnection::onSignal(GDBusConnection*,
                              const gchar* sender,
                              const gchar* object,
                              const gchar* interface,
                              const gchar* name,
                              GVariant* parameters,
                              gpointer userData)
{
    const SignalCallback& callback = getCallbackFromPointer<SignalCallback>(userData);

    LOGD("Signal: " << sender << "; " << object << "; " << interface << "; " << name);

    if (callback) {
        callback(sender, object, interface, name, parameters);
    }
}

std::string DbusConnection::introspect(const std::string& busName, const std::string& objectPath)
{
    GVariantPtr result = DbusConnection::callMethod(busName,
                                                    objectPath,
                                                    INTROSPECT_INTERFACE,
                                                    INTROSPECT_METHOD,
                                                    NULL,
                                                    "(s)");
    const gchar* xml;
    g_variant_get(result.get(), "(&s)", &xml);
    return xml;
}

void DbusConnection::registerObject(const std::string& objectPath,
                                    const std::string& objectDefinitionXml,
                                    const MethodCallCallback& callback)
{
    ScopedGError error;
    GDBusNodeInfo* nodeInfo = g_dbus_node_info_new_for_xml(objectDefinitionXml.c_str(), &error);
    if (nodeInfo != NULL && (nodeInfo->interfaces == NULL ||
                             nodeInfo->interfaces[0] == NULL ||
                             nodeInfo->interfaces[1] != NULL)) {
        g_dbus_node_info_unref(nodeInfo);
        g_set_error(&error,
                    G_MARKUP_ERROR,
                    G_MARKUP_ERROR_INVALID_CONTENT,
                    "Expected exactly one interface");
    }
    if (error) {
        error.strip();
        LOGE("Invalid xml; " << error);
        throwDbusException(error);
    }
    GDBusInterfaceInfo* interfaceInfo = nodeInfo->interfaces[0];

    GDBusInterfaceVTable vtable;
    vtable.method_call = &DbusConnection::onMethodCall;
    vtable.get_property = NULL;
    vtable.set_property = NULL;

    g_dbus_connection_register_object(mConnection,
                                      objectPath.c_str(),
                                      interfaceInfo,
                                      &vtable,
                                      createCallbackWrapper(callback, mGuard.spawn()),
                                      &deleteCallbackWrapper<MethodCallCallback>,
                                      &error);
    g_dbus_node_info_unref(nodeInfo);
    if (error) {
        error.strip();
        LOGE("Register object failed; " << error);
        throwDbusException(error);
    }
}

void DbusConnection::onMethodCall(GDBusConnection*,
                                  const gchar*,
                                  const gchar* objectPath,
                                  const gchar* interface,
                                  const gchar* method,
                                  GVariant* parameters,
                                  GDBusMethodInvocation* invocation,
                                  gpointer userData)
{
    const MethodCallCallback& callback = getCallbackFromPointer<MethodCallCallback>(userData);

    LOGD("MethodCall: " << objectPath << "; " << interface << "; " << method);

    MethodResultBuilder::Pointer resultBuilder(new MethodResultBuilderImpl(invocation));
    if (callback) {
        callback(objectPath, interface, method, parameters, resultBuilder);
    }
}

GVariantPtr DbusConnection::callMethod(const std::string& busName,
                                       const std::string& objectPath,
                                       const std::string& interface,
                                       const std::string& method,
                                       GVariant* parameters,
                                       const std::string& replyType,
                                       int timeoutMs)
{
    ScopedGError error;
    GVariant* result = g_dbus_connection_call_sync(mConnection,
                                                   busName.c_str(),
                                                   objectPath.c_str(),
                                                   interface.c_str(),
                                                   method.c_str(),
                                                   parameters,
                                                   replyType.empty() ? NULL
                                                       : G_VARIANT_TYPE(replyType.c_str()),
                                                   G_DBUS_CALL_FLAGS_NONE,
                                                   timeoutMs,
                                                   NULL,
                                                   &error);
    if (error) {
        error.strip();
        LOGE("Call method failed; " << error);
        throwDbusException(error);
    }
    return GVariantPtr(result, g_variant_unref);
}

void DbusConnection::callMethodAsync(const std::string& busName,
                                     const std::string& objectPath,
                                     const std::string& interface,
                                     const std::string& method,
                                     GVariant* parameters,
                                     const std::string& replyType,
                                     const AsyncMethodCallCallback& callback,
                                     int timeoutMs)
{
    g_dbus_connection_call(mConnection,
                           busName.c_str(),
                           objectPath.c_str(),
                           interface.c_str(),
                           method.c_str(),
                           parameters,
                           replyType.empty() ? NULL
                               : G_VARIANT_TYPE(replyType.c_str()),
                           G_DBUS_CALL_FLAGS_NONE,
                           timeoutMs,
                           NULL,
                           &DbusConnection::onAsyncReady,
                           createCallbackWrapper(callback, mGuard.spawn()));
}

void DbusConnection::onAsyncReady(GObject* source,
                                  GAsyncResult* asyncResult,
                                  gpointer userData)
{
    std::unique_ptr<void, void(*)(void*)>
        autoDeleteCallback(userData, &deleteCallbackWrapper<AsyncMethodCallCallback>);
    GDBusConnection* connection = reinterpret_cast<GDBusConnection*>(source);
    const AsyncMethodCallCallback& callback =
        getCallbackFromPointer<AsyncMethodCallCallback>(userData);

    ScopedGError error;
    GVariant* result = g_dbus_connection_call_finish(connection, asyncResult, &error);
    if (error) {
        error.strip();
        LOGE("Call method failed; " << error);
    }
    AsyncMethodCallResultImpl asyncMethodCallResult(result, error);
    if (callback) {
        try {
            callback(asyncMethodCallResult);
        } catch (DbusException& e) {
            // Drop dbus exceptions (thrown from asyncMethodCallResult.get()).
            // We can not ignore other exceptions - they must be catched inside callback,
            // otherwise std::terminate will be called.
            LOGW("Uncaugth dbus exception: " << e.what());
        }
    }
}

} // namespace dbus
