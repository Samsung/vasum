/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Jan Olszak <j.olszak@samsung.com>
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
 * @author  Jan Olszak (j.olszak@samsung.com)
 * @brief   Unit tests of the ZonesManager class
 */

#include "config.hpp"
#include "ut.hpp"

#include "zones-manager.hpp"
#include "zone-dbus-definitions.hpp"
#ifdef DBUS_CONNECTION
#include "host-dbus-definitions.hpp"
#else
#include "host-ipc-definitions.hpp"
#include <api/messages.hpp>
#include <epoll/thread-dispatcher.hpp>
#include <ipc/client.hpp>
#endif
#include "test-dbus-definitions.hpp"
// TODO: Switch to real power-manager dbus defs when they will be implemented in power-manager
#include "fake-power-manager-dbus-definitions.hpp"
#include "exception.hpp"

#include "dbus/connection.hpp"
#include "dbus/exception.hpp"
#include "utils/glib-loop.hpp"
#include "config/exception.hpp"
#include "utils/latch.hpp"
#include "utils/fs.hpp"
#include "utils/img.hpp"
#include "utils/scoped-dir.hpp"
#include "logger/logger.hpp"

#include <vector>
#include <map>
#include <memory>
#include <string>
#include <algorithm>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <boost/filesystem.hpp>

using namespace vasum;
using namespace config;
using namespace vasum::utils;
using namespace dbus;

namespace {

const std::string CONFIG_DIR = VSM_TEST_CONFIG_INSTALL_DIR "/server/ut-zones-manager";
const std::string TEST_CONFIG_PATH = CONFIG_DIR + "/test-daemon.conf";
const std::string MISSING_CONFIG_PATH = CONFIG_DIR + "/missing-daemon.conf";
const int EVENT_TIMEOUT = 5000;
//const int UNEXPECTED_EVENT_TIMEOUT = EVENT_TIMEOUT / 5;
const int TEST_DBUS_CONNECTION_ZONES_COUNT = 3;
const std::string TEST_APP_NAME = "testapp";
const std::string TEST_MESSAGE = "testmessage";
const std::string FILE_CONTENT = "File content\n"
                                 "Line 1\n"
                                 "Line 2\n";
const std::string NON_EXISTANT_ZONE_ID = "NON_EXISTANT_ZONE_ID";
const std::string ZONES_PATH = "/tmp/ut-zones"; // the same as in daemon.conf
const std::string SIMPLE_TEMPLATE = "console";
const std::string DBUS_TEMPLATE = "console-dbus";

#ifdef DBUS_CONNECTION
/**
 * Currently there is no way to propagate an error from async call
 * dropException are used to prevent system crash
 **/
DbusConnection::AsyncMethodCallCallback
dropException(const DbusConnection::AsyncMethodCallCallback& fun)
{
    return [fun](dbus::AsyncMethodCallResult& arg) -> void {
        try {
            fun(arg);
        } catch (const std::exception& ex) {
            LOGE("Got exception: " << ex.what());
        }
    };
}

class DbusAccessory {
public:
    static const int HOST_ID = 0;

    typedef std::function<void(const std::string& argument,
                               MethodResultBuilder::Pointer result
                              )> TestApiMethodCallback;
    typedef std::function<void()> VoidResultCallback;
    typedef std::function<void(const std::string& zoneId,
                               const std::string& dbusAddress)> SignalCallback;

    typedef std::map<std::string, std::string> Dbuses;

    DbusAccessory()
        : mId(0),
          mClient(DbusConnection::create(acquireAddress())),
          mNameAcquired(false),
          mPendingDisconnect(false)
    {
    }

    DbusAccessory(int id)
        : mId(id),
          mClient(DbusConnection::create(acquireAddress())),
          mNameAcquired(false),
          mPendingDisconnect(false)
    {
    }

    void setName(const std::string& name)
    {
        mClient->setName(name,
                         std::bind(&DbusAccessory::onNameAcquired, this),
                         std::bind(&DbusAccessory::onDisconnect, this));

        if(!waitForName()) {
            mClient.reset();
            throw dbus::DbusOperationException("Could not acquire name.");
        }
    }

    bool waitForName()
    {
        std::unique_lock<std::mutex> lock(mMutex);
        mNameCondition.wait(lock, [this] {return mNameAcquired || mPendingDisconnect;});
        return mNameAcquired;
    }

    void onNameAcquired()
    {
        std::unique_lock<std::mutex> lock(mMutex);
        mNameAcquired = true;
        mNameCondition.notify_one();
    }

    void onDisconnect()
    {
        std::unique_lock<std::mutex> lock(mMutex);
        mPendingDisconnect = true;
        mNameCondition.notify_one();
    }

    void signalSubscribe(const DbusConnection::SignalCallback& callback)
    {
        mClient->signalSubscribe(callback, isHost() ? api::host::BUS_NAME : api::zone::BUS_NAME);
    }

    void subscribeZoneDbusState(const SignalCallback& callback) {
        assert(isHost());
        auto onSignal = [callback] (const std::string& /*senderBusName*/,
                             const std::string& objectPath,
                             const std::string& interface,
                             const std::string& signalName,
                             GVariant* parameters) {
            if (objectPath == api::host::OBJECT_PATH &&
                interface == api::host::INTERFACE &&
                signalName == api::host::SIGNAL_ZONE_DBUS_STATE) {

                const gchar* zoneId = NULL;
                const gchar* dbusAddress = NULL;
                g_variant_get(parameters, "(&s&s)", &zoneId, &dbusAddress);
                callback(zoneId, dbusAddress);
            }
        };
        mClient->signalSubscribe(onSignal, api::host::BUS_NAME);
    }

    void emitSignal(const std::string& objectPath,
                    const std::string& interface,
                    const std::string& name,
                    GVariant* parameters)
    {
        mClient->emitSignal(objectPath, interface, name, parameters);
    }

    void callMethodNotify()
    {
        GVariant* parameters = g_variant_new("(ss)", TEST_APP_NAME.c_str(), TEST_MESSAGE.c_str());
        mClient->callMethod(api::zone::BUS_NAME,
                            api::zone::OBJECT_PATH,
                            api::zone::INTERFACE,
                            api::zone::METHOD_NOTIFY_ACTIVE_ZONE,
                            parameters,
                            "()");
    }

    std::string callMethodMove(const std::string& dest, const std::string& path)
    {
        GVariant* parameters = g_variant_new("(ss)", dest.c_str(), path.c_str());
        GVariantPtr result = mClient->callMethod(api::zone::BUS_NAME,
                                                 api::zone::OBJECT_PATH,
                                                 api::zone::INTERFACE,
                                                 api::zone::METHOD_FILE_MOVE_REQUEST,
                                                 parameters,
                                                 "(s)");

        const gchar* retcode = NULL;
        g_variant_get(result.get(), "(&s)", &retcode);
        return std::string(retcode);
    }

    void registerTestApiObject(const TestApiMethodCallback& callback)
    {
        auto handler = [callback](const std::string& objectPath,
                          const std::string& interface,
                          const std::string& methodName,
                          GVariant* parameters,
                          MethodResultBuilder::Pointer result) {
            if (objectPath == testapi::OBJECT_PATH &&
                interface == testapi::INTERFACE &&
                methodName == testapi::METHOD) {
                const gchar* argument = NULL;
                g_variant_get(parameters, "(&s)", &argument);
                if (callback) {
                    callback(argument, result);
                }
            }
        };
        mClient->registerObject(testapi::OBJECT_PATH, testapi::DEFINITION, handler);
    }

    std::string testApiProxyCall(const std::string& target, const std::string& argument)
    {
        GVariant* parameters = g_variant_new("(s)", argument.c_str());
        GVariantPtr result = proxyCall(target,
                                       testapi::BUS_NAME,
                                       testapi::OBJECT_PATH,
                                       testapi::INTERFACE,
                                       testapi::METHOD,
                                       parameters);
        const gchar* ret = NULL;
        g_variant_get(result.get(), "(&s)", &ret);
        return ret;
    }


    GVariantPtr proxyCall(const std::string& target,
                          const std::string& busName,
                          const std::string& objectPath,
                          const std::string& interface,
                          const std::string& method,
                          GVariant* parameters)
    {
        GVariant* packedParameters = g_variant_new("(sssssv)",
                                                   target.c_str(),
                                                   busName.c_str(),
                                                   objectPath.c_str(),
                                                   interface.c_str(),
                                                   method.c_str(),
                                                   parameters);
        GVariantPtr result = mClient->callMethod(isHost() ? api::host::BUS_NAME :
                                                            api::zone::BUS_NAME,
                                                 isHost() ? api::host::OBJECT_PATH :
                                                            api::zone::OBJECT_PATH,
                                                 isHost() ? api::host::INTERFACE :
                                                            api::zone::INTERFACE,
                                                 api::METHOD_PROXY_CALL,
                                                 packedParameters,
                                                 "(v)");
        GVariant* unpackedResult = NULL;
        g_variant_get(result.get(), "(v)", &unpackedResult);
        return GVariantPtr(unpackedResult, g_variant_unref);
    }

    Dbuses callMethodGetZoneDbuses()
    {
        assert(isHost());
        Dbuses dbuses;
        GVariantPtr result = mClient->callMethod(api::host::BUS_NAME,
                                                 api::host::OBJECT_PATH,
                                                 api::host::INTERFACE,
                                                 api::host::METHOD_GET_ZONE_DBUSES,
                                                 NULL,
                                                 "(a(ss))");
        GVariant* array = NULL;
        g_variant_get(result.get(), "(*)", &array);
        dbus::GVariantPtr autounref(array, g_variant_unref);
        size_t count = g_variant_n_children(array);
        for (size_t n = 0; n < count; ++n) {
            const char* zoneId = NULL;
            const char* dbusAddress = NULL;
            g_variant_get_child(array, n, "(&s&s)", &zoneId, &dbusAddress);
            dbuses.insert(Dbuses::value_type(zoneId, dbusAddress));
        }
        return dbuses;
    }

    std::vector<std::string> callMethodGetZoneIds()
    {
        assert(isHost());
        GVariantPtr result = mClient->callMethod(api::host::BUS_NAME,
                                                 api::host::OBJECT_PATH,
                                                 api::host::INTERFACE,
                                                 api::host::METHOD_GET_ZONE_ID_LIST,
                                                 NULL,
                                                 "(as)");

        GVariant* array = NULL;
        g_variant_get(result.get(), "(*)", &array);

        size_t arraySize = g_variant_n_children(array);
        std::vector<std::string> zoneIds;
        for (size_t i = 0; i < arraySize; ++i) {
            const char* id = NULL;
            g_variant_get_child(array, i, "&s", &id);
            zoneIds.push_back(id);
        }

        g_variant_unref(array);
        return zoneIds;
    }

    std::string callMethodGetActiveZoneId()
    {
        assert(isHost());
        GVariantPtr result = mClient->callMethod(api::host::BUS_NAME,
                                                 api::host::OBJECT_PATH,
                                                 api::host::INTERFACE,
                                                 api::host::METHOD_GET_ACTIVE_ZONE_ID,
                                                 NULL,
                                                 "(s)");

        const char* zoneId = NULL;
        g_variant_get(result.get(), "(&s)", &zoneId);
        return zoneId;
    }

    void callMethodSetActiveZone(const std::string& id)
    {
        assert(isHost());
        GVariant* parameters = g_variant_new("(s)", id.c_str());
        GVariantPtr result = mClient->callMethod(api::host::BUS_NAME,
                                                 api::host::OBJECT_PATH,
                                                 api::host::INTERFACE,
                                                 api::host::METHOD_SET_ACTIVE_ZONE,
                                                 parameters,
                                                 "()");

    }

    void callAsyncMethodCreateZone(const std::string& id,
                                   const std::string& templateName,
                                   const VoidResultCallback& result)
    {
        auto asyncResult = [result](dbus::AsyncMethodCallResult& asyncMethodCallResult) {
            if (g_variant_is_of_type(asyncMethodCallResult.get(), G_VARIANT_TYPE_UNIT)) {
                result();
            }
        };

        assert(isHost());
        GVariant* parameters = g_variant_new("(ss)", id.c_str(), templateName.c_str());
        mClient->callMethodAsync(api::host::BUS_NAME,
                                 api::host::OBJECT_PATH,
                                 api::host::INTERFACE,
                                 api::host::METHOD_CREATE_ZONE,
                                 parameters,
                                 "()",
                                 dropException(asyncResult));
    }

    void callAsyncMethodDestroyZone(const std::string& id,
                                    const VoidResultCallback& result)
    {
        auto asyncResult = [result](dbus::AsyncMethodCallResult& asyncMethodCallResult) {
            if (g_variant_is_of_type(asyncMethodCallResult.get(), G_VARIANT_TYPE_UNIT)) {
                result();
            }
        };

        assert(isHost());
        GVariant* parameters = g_variant_new("(s)", id.c_str());
        mClient->callMethodAsync(api::host::BUS_NAME,
                                 api::host::OBJECT_PATH,
                                 api::host::INTERFACE,
                                 api::host::METHOD_DESTROY_ZONE,
                                 parameters,
                                 "()",
                                 dropException(asyncResult));
    }

    void callAsyncMethodShutdownZone(const std::string& id,
                                     const VoidResultCallback& result)
    {
        auto asyncResult = [result](dbus::AsyncMethodCallResult& asyncMethodCallResult) {
            if (g_variant_is_of_type(asyncMethodCallResult.get(), G_VARIANT_TYPE_UNIT)) {
                result();
            }
        };

        assert(isHost());
        GVariant* parameters = g_variant_new("(s)", id.c_str());
        mClient->callMethodAsync(api::host::BUS_NAME,
                                 api::host::OBJECT_PATH,
                                 api::host::INTERFACE,
                                 api::host::METHOD_SHUTDOWN_ZONE,
                                 parameters,
                                 "()",
                                 dropException(asyncResult));
    }

    void callAsyncMethodStartZone(const std::string& id,
                                  const VoidResultCallback& result)
    {
        auto asyncResult = [result](dbus::AsyncMethodCallResult& asyncMethodCallResult) {
            if (g_variant_is_of_type(asyncMethodCallResult.get(), G_VARIANT_TYPE_UNIT)) {
                result();
            }
        };

        assert(isHost());
        GVariant* parameters = g_variant_new("(s)", id.c_str());
        mClient->callMethodAsync(api::host::BUS_NAME,
                                 api::host::OBJECT_PATH,
                                 api::host::INTERFACE,
                                 api::host::METHOD_START_ZONE,
                                 parameters,
                                 "()",
                                 dropException(asyncResult));
    }

    void callMethodLockZone(const std::string& id)
    {
        assert(isHost());
        GVariant* parameters = g_variant_new("(s)", id.c_str());
        GVariantPtr result = mClient->callMethod(api::host::BUS_NAME,
                                                 api::host::OBJECT_PATH,
                                                 api::host::INTERFACE,
                                                 api::host::METHOD_LOCK_ZONE,
                                                 parameters,
                                                 "()");
    }

    void callMethodUnlockZone(const std::string& id)
    {
        assert(isHost());
        GVariant* parameters = g_variant_new("(s)", id.c_str());
        GVariantPtr result = mClient->callMethod(api::host::BUS_NAME,
                                                 api::host::OBJECT_PATH,
                                                 api::host::INTERFACE,
                                                 api::host::METHOD_UNLOCK_ZONE,
                                                 parameters,
                                                 "()");
    }

private:
    const int mId;
    DbusConnection::Pointer mClient;
    bool mNameAcquired;
    bool mPendingDisconnect;
    std::mutex mMutex;
    std::condition_variable mNameCondition;

    bool isHost() const
    {
        return mId == HOST_ID;
    }

    std::string acquireAddress() const
    {
        if (isHost()) {
            return "unix:path=/var/run/dbus/system_bus_socket";
        }
        std::string zoneId = "zone" + std::to_string(mId);
        return "unix:path=/tmp/ut-run/" + zoneId + "/dbus/system_bus_socket";
    }
};

typedef DbusAccessory HostAccessory;
typedef DbusAccessory ZoneAccessory;

#else
//#ifdef DBUS_CONNECTION

class HostIPCAccessory {
public:
    typedef std::function<void(const std::string& argument,
                               MethodResultBuilder::Pointer result
                              )> TestApiMethodCallback;
    typedef std::function<void()> VoidResultCallback;
    typedef std::function<void(const std::string& zoneId,
                               const std::string& dbusAddress)> SignalCallback;

    typedef std::map<std::string, std::string> Dbuses;

    HostIPCAccessory()
        : mClient(mDispatcher.getPoll(), HOST_IPC_SOCKET)
    {
        mClient.start();
    }

    void subscribeZoneDbusState(const SignalCallback& callback)
    {
        auto callbackWrapper = [callback] (const ipc::PeerID, std::shared_ptr<api::DbusState>& data) {
            callback(data->first, data->second);
        };
        mClient.setSignalHandler<api::DbusState>(api::host::SIGNAL_ZONE_DBUS_STATE, callbackWrapper);
    }

    Dbuses callMethodGetZoneDbuses()
    {
        const auto out = mClient.callSync<api::Void, api::Dbuses>(api::host::METHOD_GET_ZONE_DBUSES,
                                                                  std::make_shared<api::Void>());
        Dbuses dbuses;
        for (const auto& dbus : out->values) {
            dbuses.insert(Dbuses::value_type(dbus.first, dbus.second));
        }
        return dbuses;
    }

    std::vector<std::string> callMethodGetZoneIds()
    {
        const auto out = mClient.callSync<api::Void, api::ZoneIds>(api::host::METHOD_GET_ZONE_ID_LIST,
                                                                    std::make_shared<api::Void>());
        return out->values;
    }

    std::string callMethodGetActiveZoneId()
    {
        const auto out = mClient.callSync<api::Void, api::ZoneId>(api::host::METHOD_GET_ACTIVE_ZONE_ID,
                                                                  std::make_shared<api::Void>());
        return out->value;
    }

    void callMethodSetActiveZone(const std::string& id)
    {
        mClient.callSync<api::ZoneId, api::Void>(api::host::METHOD_SET_ACTIVE_ZONE,
                                                 std::make_shared<api::ZoneId>(api::ZoneId{id}));
    }

    void callAsyncMethodCreateZone(const std::string& id,
                                   const std::string& templateName,
                                   const VoidResultCallback& result)
    {
        auto asyncResult = [result](ipc::Result<api::Void>&& out) {
            if (out.isValid()) {
                result();
            }
        };
        mClient.callAsync<api::CreateZoneIn, api::Void>(api::host::METHOD_CREATE_ZONE,
                           std::make_shared<api::CreateZoneIn>(api::CreateZoneIn{id, templateName}),
                           asyncResult);
    }

    void callAsyncMethodDestroyZone(const std::string& id,
                                    const VoidResultCallback& result)
    {
        auto asyncResult = [result](ipc::Result<api::Void>&& out) {
            if (out.isValid()) {
                result();
            }
        };
        mClient.callAsync<api::ZoneId, api::Void>(api::host::METHOD_DESTROY_ZONE,
                           std::make_shared<api::ZoneId>(api::ZoneId{id}),
                           asyncResult);
    }

    void callAsyncMethodShutdownZone(const std::string& id,
                                     const VoidResultCallback& result)
    {
        auto asyncResult = [result](ipc::Result<api::Void>&& out) {
            if (out.isValid()) {
                result();
            }
        };
        mClient.callAsync<api::ZoneId, api::Void>(api::host::METHOD_SHUTDOWN_ZONE,
                           std::make_shared<api::ZoneId>(api::ZoneId{id}),
                           asyncResult);
    }

    void callAsyncMethodStartZone(const std::string& id,
                                  const VoidResultCallback& result)
    {
        auto asyncResult = [result](ipc::Result<api::Void>&& out) {
            if (out.isValid()) {
                result();
            }
        };
        mClient.callAsync<api::ZoneId, api::Void>(api::host::METHOD_START_ZONE,
                           std::make_shared<api::ZoneId>(api::ZoneId{id}),
                           asyncResult);
    }

    void callMethodLockZone(const std::string& id)
    {
        mClient.callSync<api::ZoneId, api::Void>(api::host::METHOD_LOCK_ZONE,
                                                  std::make_shared<api::ZoneId>(api::ZoneId{id}),
                                                  EVENT_TIMEOUT*10); //Prevent from IPCTimeoutException see LockUnlockZone
    }

    void callMethodUnlockZone(const std::string& id)
    {
        mClient.callSync<api::ZoneId, api::Void>(api::host::METHOD_UNLOCK_ZONE,
                                                  std::make_shared<api::ZoneId>(api::ZoneId{id}),
                                                  EVENT_TIMEOUT*10); //Prevent from IPCTimeoutException see LockUnlockZone
    }

private:
    epoll::ThreadDispatcher mDispatcher;
    ipc::Client mClient;
};

class ZoneDbusAccessory {
public:
    typedef std::function<void(const std::string& argument,
                               MethodResultBuilder::Pointer result
                              )> TestApiMethodCallback;

    typedef std::map<std::string, std::string> Dbuses;

    ZoneDbusAccessory(int id)
        : mId(id),
          mClient(DbusConnection::create(acquireAddress())),
          mNameAcquired(false),
          mPendingDisconnect(false)
    {
    }

    void setName(const std::string& name)
    {
        mClient->setName(name,
                         std::bind(&ZoneDbusAccessory::onNameAcquired, this),
                         std::bind(&ZoneDbusAccessory::onDisconnect, this));

        if(!waitForName()) {
            mClient.reset();
            throw dbus::DbusOperationException("Could not acquire name.");
        }
    }

    bool waitForName()
    {
        std::unique_lock<std::mutex> lock(mMutex);
        mNameCondition.wait(lock, [this] {return mNameAcquired || mPendingDisconnect;});
        return mNameAcquired;
    }

    void onNameAcquired()
    {
        std::unique_lock<std::mutex> lock(mMutex);
        mNameAcquired = true;
        mNameCondition.notify_one();
    }

    void onDisconnect()
    {
        std::unique_lock<std::mutex> lock(mMutex);
        mPendingDisconnect = true;
        mNameCondition.notify_one();
    }

    void signalSubscribe(const DbusConnection::SignalCallback& callback)
    {
        mClient->signalSubscribe(callback, api::zone::BUS_NAME);
    }

    void emitSignal(const std::string& objectPath,
                    const std::string& interface,
                    const std::string& name,
                    GVariant* parameters)
    {
        mClient->emitSignal(objectPath, interface, name, parameters);
    }

    void callMethodNotify()
    {
        GVariant* parameters = g_variant_new("(ss)", TEST_APP_NAME.c_str(), TEST_MESSAGE.c_str());
        mClient->callMethod(api::zone::BUS_NAME,
                            api::zone::OBJECT_PATH,
                            api::zone::INTERFACE,
                            api::zone::METHOD_NOTIFY_ACTIVE_ZONE,
                            parameters,
                            "()");
    }

    std::string callMethodMove(const std::string& dest, const std::string& path)
    {
        GVariant* parameters = g_variant_new("(ss)", dest.c_str(), path.c_str());
        GVariantPtr result = mClient->callMethod(api::zone::BUS_NAME,
                                                 api::zone::OBJECT_PATH,
                                                 api::zone::INTERFACE,
                                                 api::zone::METHOD_FILE_MOVE_REQUEST,
                                                 parameters,
                                                 "(s)");

        const gchar* retcode = NULL;
        g_variant_get(result.get(), "(&s)", &retcode);
        return std::string(retcode);
    }

    void registerTestApiObject(const TestApiMethodCallback& callback)
    {
        auto handler = [callback](const std::string& objectPath,
                          const std::string& interface,
                          const std::string& methodName,
                          GVariant* parameters,
                          MethodResultBuilder::Pointer result) {
            if (objectPath == testapi::OBJECT_PATH &&
                interface == testapi::INTERFACE &&
                methodName == testapi::METHOD) {
                const gchar* argument = NULL;
                g_variant_get(parameters, "(&s)", &argument);
                if (callback) {
                    callback(argument, result);
                }
            }
        };
        mClient->registerObject(testapi::OBJECT_PATH, testapi::DEFINITION, handler);
    }

    std::string testApiProxyCall(const std::string& target, const std::string& argument)
    {
        GVariant* parameters = g_variant_new("(s)", argument.c_str());
        GVariantPtr result = proxyCall(target,
                                       testapi::BUS_NAME,
                                       testapi::OBJECT_PATH,
                                       testapi::INTERFACE,
                                       testapi::METHOD,
                                       parameters);
        const gchar* ret = NULL;
        g_variant_get(result.get(), "(&s)", &ret);
        return ret;
    }


    GVariantPtr proxyCall(const std::string& target,
                          const std::string& busName,
                          const std::string& objectPath,
                          const std::string& interface,
                          const std::string& method,
                          GVariant* parameters)
    {
        GVariant* packedParameters = g_variant_new("(sssssv)",
                                                   target.c_str(),
                                                   busName.c_str(),
                                                   objectPath.c_str(),
                                                   interface.c_str(),
                                                   method.c_str(),
                                                   parameters);
        GVariantPtr result = mClient->callMethod(api::zone::BUS_NAME,
                                                 api::zone::OBJECT_PATH,
                                                 api::zone::INTERFACE,
                                                 api::METHOD_PROXY_CALL,
                                                 packedParameters,
                                                 "(v)");
        GVariant* unpackedResult = NULL;
        g_variant_get(result.get(), "(v)", &unpackedResult);
        return GVariantPtr(unpackedResult, g_variant_unref);
    }

private:
    const int mId;
    DbusConnection::Pointer mClient;
    bool mNameAcquired;
    bool mPendingDisconnect;
    std::mutex mMutex;
    std::condition_variable mNameCondition;

    std::string acquireAddress() const
    {
        std::string zoneId = "zone" + std::to_string(mId);
        return "unix:path=/tmp/ut-run/" + zoneId + "/dbus/system_bus_socket";
    }
};

typedef HostIPCAccessory HostAccessory;
typedef ZoneDbusAccessory ZoneAccessory;

#endif //DBUS_CONNECTION

template<class Predicate>
bool spinWaitFor(int timeoutMs, Predicate pred)
{
    auto until = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    while (!pred()) {
        if (std::chrono::steady_clock::now() >= until) {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return true;
}

struct Fixture {
    vasum::utils::ScopedGlibLoop mLoop;

    utils::ScopedDir mZonesPathGuard;
    utils::ScopedDir mRunGuard;

    Fixture()
        : mZonesPathGuard(ZONES_PATH)
        , mRunGuard("/tmp/ut-run")
    {}
};

} // namespace


BOOST_FIXTURE_TEST_SUITE(ZonesManagerSuite, Fixture)

BOOST_AUTO_TEST_CASE(ConstructorDestructor)
{
    std::unique_ptr<ZonesManager> cm;
    cm.reset(new ZonesManager(TEST_CONFIG_PATH));
    cm.reset();
}

BOOST_AUTO_TEST_CASE(MissingConfig)
{
    BOOST_REQUIRE_EXCEPTION(ZonesManager{MISSING_CONFIG_PATH},
                            ConfigException,
                            WhatEquals("Could not load " + MISSING_CONFIG_PATH));
}

BOOST_AUTO_TEST_CASE(Create)
{
    ZonesManager cm(TEST_CONFIG_PATH);
    cm.createZone("zone1", SIMPLE_TEMPLATE);
    cm.createZone("zone2", SIMPLE_TEMPLATE);
}

BOOST_AUTO_TEST_CASE(StartStop)
{
    ZonesManager cm(TEST_CONFIG_PATH);
    cm.createZone("zone1", SIMPLE_TEMPLATE);
    cm.createZone("zone2", SIMPLE_TEMPLATE);

    cm.restoreAll();
    BOOST_CHECK_EQUAL(cm.getRunningForegroundZoneId(), "zone1");
    cm.shutdownAll();
    BOOST_CHECK_EQUAL(cm.getRunningForegroundZoneId(), "");
}

BOOST_AUTO_TEST_CASE(DetachOnExit)
{
    {
        ZonesManager cm(TEST_CONFIG_PATH);
        cm.createZone("zone1", SIMPLE_TEMPLATE);
        cm.createZone("zone2", SIMPLE_TEMPLATE);
        cm.restoreAll();
        BOOST_CHECK_EQUAL(cm.getRunningForegroundZoneId(), "zone1");
        cm.setZonesDetachOnExit();
    }
    {
        ZonesManager cm(TEST_CONFIG_PATH);
        cm.restoreAll();
        BOOST_CHECK_EQUAL(cm.getRunningForegroundZoneId(), "zone1");
    }
}

BOOST_AUTO_TEST_CASE(Focus)
{
    ZonesManager cm(TEST_CONFIG_PATH);
    cm.createZone("zone1", SIMPLE_TEMPLATE);
    cm.createZone("zone2", SIMPLE_TEMPLATE);
    cm.createZone("zone3", SIMPLE_TEMPLATE);
    cm.restoreAll();

    BOOST_CHECK(cm.getRunningForegroundZoneId() == "zone1");
    cm.focus("zone2");
    BOOST_CHECK(cm.getRunningForegroundZoneId() == "zone2");
    cm.focus("zone1");
    BOOST_CHECK(cm.getRunningForegroundZoneId() == "zone1");
    cm.focus("zone3");
    BOOST_CHECK(cm.getRunningForegroundZoneId() == "zone3");
}

BOOST_AUTO_TEST_CASE(NotifyActiveZone)
{
    ZonesManager cm(TEST_CONFIG_PATH);
    cm.createZone("zone1", DBUS_TEMPLATE);
    cm.createZone("zone2", DBUS_TEMPLATE);
    cm.createZone("zone3", DBUS_TEMPLATE);
    cm.restoreAll();

    Latch signalReceivedLatch;
    std::map<int, std::vector<std::string>> signalReceivedSourcesMap;

    std::map<int, std::unique_ptr<ZoneAccessory>> dbuses;
    for (int i = 1; i <= TEST_DBUS_CONNECTION_ZONES_COUNT; ++i) {
        dbuses[i] = std::unique_ptr<ZoneAccessory>(new ZoneAccessory(i));
    }

    auto handler = [](Latch& latch,
                      std::vector<std::string>& receivedSignalSources,
                      const std::string& /*senderBusName*/,
                      const std::string& objectPath,
                      const std::string& interface,
                      const std::string& signalName,
                      GVariant* parameters)
        {
            if (objectPath == api::zone::OBJECT_PATH &&
                interface == api::zone::INTERFACE &&
                signalName == api::zone::SIGNAL_NOTIFICATION &&
                g_variant_is_of_type(parameters, G_VARIANT_TYPE("(sss)"))) {

                const gchar* zone = NULL;
                const gchar* application = NULL;
                const gchar* message = NULL;
                g_variant_get(parameters, "(&s&s&s)", &zone, &application, &message);
                receivedSignalSources.push_back(zone);
                if (application == TEST_APP_NAME && message == TEST_MESSAGE) {
                    latch.set();
                }
            }
        };

    using namespace std::placeholders;
    for (int i = 1; i <= TEST_DBUS_CONNECTION_ZONES_COUNT; ++i) {
        dbuses[i]->signalSubscribe(std::bind(handler,
                                             std::ref(signalReceivedLatch),
                                             std::ref(signalReceivedSourcesMap[i]),
                                             _1, _2, _3, _4, _5));
    }
    for (auto& dbus : dbuses) {
        dbus.second->callMethodNotify();
    }

    BOOST_REQUIRE(signalReceivedLatch.waitForN(dbuses.size() - 1u, EVENT_TIMEOUT));
    BOOST_REQUIRE(signalReceivedLatch.empty());

    //check if there are no signals that was received more than once
    for (const auto& source : signalReceivedSourcesMap[1]) {
        BOOST_CHECK_EQUAL(std::count(signalReceivedSourcesMap[1].begin(),
                                     signalReceivedSourcesMap[1].end(),
                                     source), 1);
    }
    //check if all signals was received by active zone
    BOOST_CHECK_EQUAL(signalReceivedSourcesMap[1].size(), dbuses.size() - 1);
    //check if no signals was received by inactive zone
    for (size_t i = 2; i <= dbuses.size(); ++i) {
        BOOST_CHECK(signalReceivedSourcesMap[i].empty());
    }

    dbuses.clear();
}

BOOST_AUTO_TEST_CASE(DisplayOff)
{
    ZonesManager cm(TEST_CONFIG_PATH);
    cm.createZone("zone1", DBUS_TEMPLATE);
    cm.createZone("zone2", DBUS_TEMPLATE);
    cm.createZone("zone3", DBUS_TEMPLATE);
    cm.restoreAll();

    std::vector<std::unique_ptr<ZoneAccessory>> clients;
    for (int i = 1; i <= TEST_DBUS_CONNECTION_ZONES_COUNT; ++i) {
        clients.push_back(std::unique_ptr<ZoneAccessory>(new ZoneAccessory(i)));
    }

    for (auto& client : clients) {
        client->setName(fake_power_manager_api::BUS_NAME);
    }

    auto isDefaultFocused = [&cm]() -> bool {
        return cm.getRunningForegroundZoneId() == "zone1";
    };

    for (auto& client : clients) {
        // TEST SWITCHING TO DEFAULT ZONE
        // focus non-default zone
        cm.focus("zone3");

        // emit signal from dbus connection
        client->emitSignal(fake_power_manager_api::OBJECT_PATH,
                           fake_power_manager_api::INTERFACE,
                           fake_power_manager_api::SIGNAL_DISPLAY_OFF,
                           nullptr);

        // check if default zone has focus
        BOOST_CHECK(spinWaitFor(EVENT_TIMEOUT, isDefaultFocused));
    }
}

BOOST_AUTO_TEST_CASE(MoveFile)
{
    ZonesManager cm(TEST_CONFIG_PATH);
    cm.createZone("zone1", DBUS_TEMPLATE);
    cm.createZone("zone2", DBUS_TEMPLATE);
    cm.createZone("zone3", DBUS_TEMPLATE);
    cm.restoreAll();

    Latch notificationLatch;
    std::string notificationSource;
    std::string notificationPath;
    std::string notificationRetcode;

    std::map<int, std::unique_ptr<ZoneAccessory>> dbuses;
    for (int i = 1; i <= 2; ++i) {
        dbuses[i] = std::unique_ptr<ZoneAccessory>(new ZoneAccessory(i));
    }

    auto handler = [&](const std::string& /*senderBusName*/,
                       const std::string& objectPath,
                       const std::string& interface,
                       const std::string& signalName,
                       GVariant* parameters)
        {
            if (objectPath == api::zone::OBJECT_PATH &&
                interface == api::zone::INTERFACE &&
                signalName == api::zone::SIGNAL_NOTIFICATION &&
                g_variant_is_of_type(parameters, G_VARIANT_TYPE("(sss)"))) {

                const gchar* source = NULL;
                const gchar* path = NULL;
                const gchar* retcode = NULL;
                g_variant_get(parameters, "(&s&s&s)", &source, &path, &retcode);

                notificationSource = source;
                notificationPath = path;
                notificationRetcode = retcode;
                notificationLatch.set();
            }
        };

    // subscribe the second (destination) zone for notifications
    dbuses.at(2)->signalSubscribe(handler);

    const std::string TMP = "/tmp/ut-zones";
    const std::string NO_PATH = "path_doesnt_matter_here";
    const std::string BUGGY_PATH = TMP + "/this_file_does_not_exist";
    const std::string BUGGY_ZONE = "this-zone-does-not-exist";
    const std::string ZONE1 = "zone1";
    const std::string ZONE2 = "zone2";
    const std::string ZONE1PATH = TMP + "/" + ZONE1 + TMP;
    const std::string ZONE2PATH = TMP + "/" + ZONE2 + TMP;

    // sending to a non existing zone
    BOOST_CHECK_EQUAL(dbuses.at(1)->callMethodMove(BUGGY_ZONE, NO_PATH),
                      api::zone::FILE_MOVE_DESTINATION_NOT_FOUND);
    BOOST_CHECK(notificationLatch.empty());

    // sending to self
    BOOST_CHECK_EQUAL(dbuses.at(1)->callMethodMove(ZONE1, NO_PATH),
                      api::zone::FILE_MOVE_WRONG_DESTINATION);
    BOOST_CHECK(notificationLatch.empty());

    // no permission to send
    BOOST_CHECK_EQUAL(dbuses.at(1)->callMethodMove(ZONE2, "/etc/secret1"),
                      api::zone::FILE_MOVE_NO_PERMISSIONS_SEND);
    BOOST_CHECK(notificationLatch.empty());

    // no permission to receive
    // TODO uncomment this after adding an api to change 'permittedTo*' config
    //BOOST_CHECK_EQUAL(dbuses.at(1)->callMethodMove(ZONE2, "/etc/secret2"),
    //                  api::zone::FILE_MOVE_NO_PERMISSIONS_RECEIVE);
    //BOOST_CHECK(notificationLatch.empty());

    // non existing file
    BOOST_CHECK_EQUAL(dbuses.at(1)->callMethodMove(ZONE2, BUGGY_PATH),
                      api::zone::FILE_MOVE_FAILED);
    BOOST_CHECK(notificationLatch.empty());

    // a working scenario
    namespace fs = boost::filesystem;
    boost::system::error_code ec;
    fs::remove_all(ZONE1PATH, ec);
    fs::remove_all(ZONE2PATH, ec);
    BOOST_REQUIRE(fs::create_directories(ZONE1PATH, ec));
    BOOST_REQUIRE(fs::create_directories(ZONE2PATH, ec));
    BOOST_REQUIRE(utils::saveFileContent(ZONE1PATH + "/file", FILE_CONTENT));

    BOOST_CHECK_EQUAL(dbuses.at(1)->callMethodMove(ZONE2, TMP + "/file"),
                      api::zone::FILE_MOVE_SUCCEEDED);
    BOOST_REQUIRE(notificationLatch.wait(EVENT_TIMEOUT));
    BOOST_REQUIRE(notificationLatch.empty());
    BOOST_CHECK_EQUAL(notificationSource, ZONE1);
    BOOST_CHECK_EQUAL(notificationPath, TMP + "/file");
    BOOST_CHECK_EQUAL(notificationRetcode, api::zone::FILE_MOVE_SUCCEEDED);
    BOOST_CHECK(!fs::exists(ZONE1PATH + "/file"));
    BOOST_CHECK_EQUAL(utils::readFileContent(ZONE2PATH + "/file"), FILE_CONTENT);

    fs::remove_all(ZONE1PATH, ec);
    fs::remove_all(ZONE2PATH, ec);
}

BOOST_AUTO_TEST_CASE(AllowSwitchToDefault)
{
    ZonesManager cm(TEST_CONFIG_PATH);
    cm.createZone("zone1", DBUS_TEMPLATE);
    cm.createZone("zone2", DBUS_TEMPLATE);
    cm.createZone("zone3", DBUS_TEMPLATE);
    cm.restoreAll();

    std::vector<std::unique_ptr<ZoneAccessory>> clients;
    for (int i = 1; i <= TEST_DBUS_CONNECTION_ZONES_COUNT; ++i) {
        clients.push_back(std::unique_ptr<ZoneAccessory>(new ZoneAccessory(i)));
    }

    for (auto& client : clients) {
        client->setName(fake_power_manager_api::BUS_NAME);
    }

    auto isDefaultFocused = [&cm]() -> bool {
        return cm.getRunningForegroundZoneId() == "zone1";
    };

    for (auto& client : clients) {
        // focus non-default zone with allowed switching
        cm.focus("zone3");

        // emit signal from dbus connection
        client->emitSignal(fake_power_manager_api::OBJECT_PATH,
                           fake_power_manager_api::INTERFACE,
                           fake_power_manager_api::SIGNAL_DISPLAY_OFF,
                           nullptr);

        // check if default zone has focus
        BOOST_CHECK(spinWaitFor(EVENT_TIMEOUT, isDefaultFocused));

        // focus non-default zone with disabled switching
        cm.focus("zone2");

        // emit signal from dbus connection
        client->emitSignal(fake_power_manager_api::OBJECT_PATH,
                           fake_power_manager_api::INTERFACE,
                           fake_power_manager_api::SIGNAL_DISPLAY_OFF,
                           nullptr);

        // now default zone should not be focused
        // TODO uncomment this after adding an api to change 'switchToDefaultAfterTimeout'
        //BOOST_CHECK(!spinWaitFor(UNEXPECTED_EVENT_TIMEOUT, isDefaultFocused));
    }
}

#ifdef DBUS_CONNECTION
BOOST_AUTO_TEST_CASE(ProxyCall)
{
    ZonesManager cm(TEST_CONFIG_PATH);
    cm.createZone("zone1", DBUS_TEMPLATE);
    cm.createZone("zone2", DBUS_TEMPLATE);
    cm.createZone("zone3", DBUS_TEMPLATE);
    cm.restoreAll();

    std::map<int, std::unique_ptr<DbusAccessory>> dbuses;
    dbuses[0] = std::unique_ptr<DbusAccessory>(new HostAccessory());
    for (int i = 1; i <= TEST_DBUS_CONNECTION_ZONES_COUNT; ++i) {
        dbuses[i] = std::unique_ptr<DbusAccessory>(new ZoneAccessory(i));
    }

    for (auto& dbus : dbuses) {
        dbus.second->setName(testapi::BUS_NAME);

        const int id = dbus.first;
        auto handler = [id](const std::string& argument, MethodResultBuilder::Pointer result) {
            if (argument.empty()) {
                result->setError("org.tizen.vasum.Error.Test", "Test error");
            } else {
                std::string ret = "reply from " + std::to_string(id) + ": " + argument;
                result->set(g_variant_new("(s)", ret.c_str()));
            }
        };
        dbus.second->registerTestApiObject(handler);
    }

    // host -> zone2
    BOOST_CHECK_EQUAL("reply from 2: param1",
                      dbuses.at(0)->testApiProxyCall("zone2",
                                                     "param1"));

    // host -> host
    BOOST_CHECK_EQUAL("reply from 0: param2",
                      dbuses.at(0)->testApiProxyCall("host",
                                                     "param2"));

    // zone1 -> host
    BOOST_CHECK_EQUAL("reply from 0: param3",
                      dbuses.at(1)->testApiProxyCall("host",
                                                     "param3"));

    // zone1 -> zone2
    BOOST_CHECK_EQUAL("reply from 2: param4",
                      dbuses.at(1)->testApiProxyCall("zone2",
                                                     "param4"));

    // zone2 -> zone2
    BOOST_CHECK_EQUAL("reply from 2: param5",
                      dbuses.at(2)->testApiProxyCall("zone2",
                                                     "param5"));

    // host -> unknown
    BOOST_CHECK_EXCEPTION(dbuses.at(0)->testApiProxyCall("unknown", "param"),
                          DbusCustomException,
                          WhatEquals("Unknown proxy call target"));

    // forwarding error
    BOOST_CHECK_EXCEPTION(dbuses.at(0)->testApiProxyCall("host", ""),
                          DbusCustomException,
                          WhatEquals("Test error"));

    // forbidden call
    BOOST_CHECK_EXCEPTION(dbuses.at(0)->proxyCall("host",
                                              "org.fake",
                                              "/a/b",
                                              "c.d",
                                              "foo",
                                              g_variant_new("(s)", "arg")),
                          DbusCustomException,
                          WhatEquals("Proxy call forbidden"));
}
#endif // DBUS_CONNECTION

namespace {
    const HostAccessory::Dbuses EXPECTED_DBUSES_NONE = {
        {"zone1", ""},
        {"zone2", ""},
        {"zone3", ""}};

    const HostAccessory::Dbuses EXPECTED_DBUSES_ALL = {
        {"zone1",
         "unix:path=/tmp/ut-run/zone1/dbus/system_bus_socket"},
        {"zone2",
         "unix:path=/tmp/ut-run/zone2/dbus/system_bus_socket"},
        {"zone3",
         "unix:path=/tmp/ut-run/zone3/dbus/system_bus_socket"}};
} // namespace

BOOST_AUTO_TEST_CASE(GetZoneDbuses)
{
    ZonesManager cm(TEST_CONFIG_PATH);
    HostAccessory host;
    cm.createZone("zone1", DBUS_TEMPLATE);
    cm.createZone("zone2", DBUS_TEMPLATE);
    cm.createZone("zone3", DBUS_TEMPLATE);

    BOOST_CHECK(EXPECTED_DBUSES_NONE == host.callMethodGetZoneDbuses());
    cm.restoreAll();
    BOOST_CHECK(EXPECTED_DBUSES_ALL == host.callMethodGetZoneDbuses());
    cm.shutdownAll();
    BOOST_CHECK(EXPECTED_DBUSES_NONE == host.callMethodGetZoneDbuses());
}

BOOST_AUTO_TEST_CASE(GetZoneDbusesNoDbus)
{
    ZonesManager cm(TEST_CONFIG_PATH);
    HostAccessory host;
    cm.createZone("zone1", SIMPLE_TEMPLATE);
    cm.createZone("zone2", SIMPLE_TEMPLATE);
    cm.createZone("zone3", SIMPLE_TEMPLATE);

    BOOST_CHECK(EXPECTED_DBUSES_NONE == host.callMethodGetZoneDbuses());
    cm.restoreAll();
    BOOST_CHECK(EXPECTED_DBUSES_NONE == host.callMethodGetZoneDbuses());
    cm.shutdownAll();
    BOOST_CHECK(EXPECTED_DBUSES_NONE == host.callMethodGetZoneDbuses());
}

BOOST_AUTO_TEST_CASE(ZoneDbusesSignals)
{
    Latch signalLatch;
    HostAccessory::Dbuses collectedDbuses;
    std::mutex collectedDbusesMutex;

    auto onSignal = [&] (const std::string& zoneId, const std::string& dbusAddress) {
        std::unique_lock<std::mutex> lock(collectedDbusesMutex);
        collectedDbuses.insert(HostAccessory::Dbuses::value_type(zoneId, dbusAddress));
        signalLatch.set();
    };

    {
        ZonesManager cm(TEST_CONFIG_PATH);
        HostAccessory host;
        host.subscribeZoneDbusState(onSignal);
        cm.createZone("zone1", DBUS_TEMPLATE);
        cm.createZone("zone2", DBUS_TEMPLATE);
        cm.createZone("zone3", DBUS_TEMPLATE);

        BOOST_CHECK(signalLatch.empty());
        BOOST_CHECK(collectedDbuses.empty());

        cm.restoreAll();

        BOOST_REQUIRE(signalLatch.waitForN(TEST_DBUS_CONNECTION_ZONES_COUNT, EVENT_TIMEOUT));
        BOOST_CHECK(signalLatch.empty());
        std::unique_lock<std::mutex> lock(collectedDbusesMutex);
        BOOST_CHECK(EXPECTED_DBUSES_ALL == collectedDbuses);
        collectedDbuses.clear();
    }
}


BOOST_AUTO_TEST_CASE(GetZoneIds)
{
    ZonesManager cm(TEST_CONFIG_PATH);
    cm.createZone("zone1", SIMPLE_TEMPLATE);
    cm.createZone("zone2", SIMPLE_TEMPLATE);
    cm.createZone("zone3", SIMPLE_TEMPLATE);

    HostAccessory host;

    std::vector<std::string> zoneIds = {"zone1",
                                        "zone2",
                                        "zone3"};
    std::vector<std::string> returnedIds = host.callMethodGetZoneIds();

    BOOST_CHECK(returnedIds == zoneIds);// order should be preserved
}

BOOST_AUTO_TEST_CASE(GetActiveZoneId)
{
    ZonesManager cm(TEST_CONFIG_PATH);
    cm.createZone("zone1", SIMPLE_TEMPLATE);
    cm.createZone("zone2", SIMPLE_TEMPLATE);
    cm.createZone("zone3", SIMPLE_TEMPLATE);
    cm.restoreAll();

    HostAccessory host;

    std::vector<std::string> zoneIds = {"zone1",
                                        "zone2",
                                        "zone3"};

    for (const std::string& zoneId: zoneIds){
        cm.focus(zoneId);
        BOOST_CHECK(host.callMethodGetActiveZoneId() == zoneId);
    }

    cm.shutdownAll();
    BOOST_CHECK(host.callMethodGetActiveZoneId() == "");
}

BOOST_AUTO_TEST_CASE(SetActiveZone)
{
    ZonesManager cm(TEST_CONFIG_PATH);
    cm.createZone("zone1", SIMPLE_TEMPLATE);
    cm.createZone("zone2", SIMPLE_TEMPLATE);
    cm.createZone("zone3", SIMPLE_TEMPLATE);
    cm.restoreAll();

    HostAccessory host;

    std::vector<std::string> zoneIds = {"zone1",
                                        "zone2",
                                        "zone3"};

    for (const std::string& zoneId: zoneIds){
        host.callMethodSetActiveZone(zoneId);
        BOOST_CHECK(host.callMethodGetActiveZoneId() == zoneId);
    }

    BOOST_REQUIRE_EXCEPTION(host.callMethodSetActiveZone(NON_EXISTANT_ZONE_ID),
                            std::exception, //TODO: exception should be more specific
                            WhatEquals("No such zone id"));

    cm.shutdownAll();
    BOOST_REQUIRE_EXCEPTION(host.callMethodSetActiveZone("zone1"),
                            std::exception, //TODO: exception should be more specific
                            WhatEquals("Could not activate stopped or paused zone"));
}

BOOST_AUTO_TEST_CASE(CreateDestroyZone)
{
    const std::string zone1 = "test1";
    const std::string zone2 = "test2";
    const std::string zone3 = "test3";

    ZonesManager cm(TEST_CONFIG_PATH);
    cm.restoreAll();

    BOOST_CHECK_EQUAL(cm.getRunningForegroundZoneId(), "");

    Latch callDone;
    auto resultCallback = [&]() {
        callDone.set();
    };

    HostAccessory host;

    // create zone1
    host.callAsyncMethodCreateZone(zone1, SIMPLE_TEMPLATE, resultCallback);
    BOOST_REQUIRE(callDone.wait(EVENT_TIMEOUT));

    // create zone2
    host.callAsyncMethodCreateZone(zone2, SIMPLE_TEMPLATE, resultCallback);
    BOOST_REQUIRE(callDone.wait(EVENT_TIMEOUT));

    // create zone3
    host.callAsyncMethodCreateZone(zone3, SIMPLE_TEMPLATE, resultCallback);
    BOOST_REQUIRE(callDone.wait(EVENT_TIMEOUT));

    cm.restoreAll();

    BOOST_CHECK_EQUAL(cm.getRunningForegroundZoneId(), zone1);
    cm.focus(zone3);
    BOOST_CHECK_EQUAL(cm.getRunningForegroundZoneId(), zone3);

    // destroy zone2
    host.callAsyncMethodDestroyZone(zone2, resultCallback);
    BOOST_REQUIRE(callDone.wait(EVENT_TIMEOUT));
    BOOST_CHECK_EQUAL(cm.getRunningForegroundZoneId(), zone3);

    // destroy zone3
    host.callAsyncMethodDestroyZone(zone3, resultCallback);
    BOOST_REQUIRE(callDone.wait(EVENT_TIMEOUT));
    BOOST_CHECK_EQUAL(cm.getRunningForegroundZoneId(), zone1);

    // destroy zone1
    host.callAsyncMethodDestroyZone(zone1, resultCallback);
    BOOST_REQUIRE(callDone.wait(EVENT_TIMEOUT));
    BOOST_CHECK_EQUAL(cm.getRunningForegroundZoneId(), "");
}

BOOST_AUTO_TEST_CASE(CreateDestroyZonePersistence)
{
    const std::string zone = "test1";

    Latch callDone;
    auto resultCallback = [&]() {
        callDone.set();
    };

    auto getZoneIds = []() -> std::vector<std::string> {
        ZonesManager cm(TEST_CONFIG_PATH);
        cm.restoreAll();

        HostAccessory host;
        return host.callMethodGetZoneIds();
    };

    BOOST_CHECK(getZoneIds().empty());

    // create zone
    {
        ZonesManager cm(TEST_CONFIG_PATH);
        HostAccessory host;
        host.callAsyncMethodCreateZone(zone, SIMPLE_TEMPLATE, resultCallback);
        BOOST_REQUIRE(callDone.wait(EVENT_TIMEOUT));
    }

    {
        auto ids = getZoneIds();
        BOOST_CHECK_EQUAL(1, ids.size());
        BOOST_CHECK(ids.at(0) == zone);
    }

    // destroy zone
    {
        ZonesManager cm(TEST_CONFIG_PATH);
        HostAccessory host;
        host.callAsyncMethodDestroyZone(zone, resultCallback);
        BOOST_REQUIRE(callDone.wait(EVENT_TIMEOUT));
    }

    BOOST_CHECK(getZoneIds().empty());
}

BOOST_AUTO_TEST_CASE(ZoneStatePersistence)
{
    const std::string zone1 = "zone1";
    const std::string zone2 = "zone2";
    const std::string zone3 = "zone3";
    const std::string zone4 = "zone4";
    const std::string zone5 = "zone5";

    Latch callDone;
    auto resultCallback = [&]() {
        callDone.set();
    };

    // firts run
    {
        ZonesManager cm(TEST_CONFIG_PATH);
        HostAccessory host;

        // zone1 - created
        host.callAsyncMethodCreateZone(zone1, SIMPLE_TEMPLATE, resultCallback);
        BOOST_REQUIRE(callDone.wait(EVENT_TIMEOUT));

        // zone2 - started
        host.callAsyncMethodCreateZone(zone2, SIMPLE_TEMPLATE, resultCallback);
        BOOST_REQUIRE(callDone.wait(EVENT_TIMEOUT));
        host.callAsyncMethodStartZone(zone2, resultCallback);
        BOOST_REQUIRE(callDone.wait(EVENT_TIMEOUT));
        BOOST_CHECK(cm.isRunning(zone2));

        // zone3 - started then paused
        host.callAsyncMethodCreateZone(zone3, SIMPLE_TEMPLATE, resultCallback);
        BOOST_REQUIRE(callDone.wait(EVENT_TIMEOUT));
        host.callAsyncMethodStartZone(zone3, resultCallback);
        BOOST_REQUIRE(callDone.wait(EVENT_TIMEOUT));
        host.callMethodLockZone(zone3);
        BOOST_CHECK(cm.isPaused(zone3));

        // zone4 - started then stopped
        host.callAsyncMethodCreateZone(zone4, SIMPLE_TEMPLATE, resultCallback);
        BOOST_REQUIRE(callDone.wait(EVENT_TIMEOUT));
        host.callAsyncMethodStartZone(zone4, resultCallback);
        BOOST_REQUIRE(callDone.wait(EVENT_TIMEOUT));
        host.callAsyncMethodShutdownZone(zone4, resultCallback);
        BOOST_REQUIRE(callDone.wait(EVENT_TIMEOUT));
        BOOST_CHECK(cm.isStopped(zone4));

        // zone5 - started then stopped then started
        host.callAsyncMethodCreateZone(zone5, SIMPLE_TEMPLATE, resultCallback);
        BOOST_REQUIRE(callDone.wait(EVENT_TIMEOUT));
        host.callAsyncMethodStartZone(zone5, resultCallback);
        BOOST_REQUIRE(callDone.wait(EVENT_TIMEOUT));
        host.callAsyncMethodShutdownZone(zone5, resultCallback);
        BOOST_REQUIRE(callDone.wait(EVENT_TIMEOUT));
        host.callAsyncMethodStartZone(zone5, resultCallback);
        BOOST_REQUIRE(callDone.wait(EVENT_TIMEOUT));
        BOOST_CHECK(cm.isRunning(zone5));
    }

    // second run
    {
        ZonesManager cm(TEST_CONFIG_PATH);
        cm.restoreAll();

        BOOST_CHECK(cm.isRunning(zone1)); // because the default json value
        BOOST_CHECK(cm.isRunning(zone2));
        BOOST_CHECK(cm.isPaused(zone3));
        BOOST_CHECK(cm.isStopped(zone4));
        BOOST_CHECK(cm.isRunning(zone5));
    }
}

BOOST_AUTO_TEST_CASE(StartShutdownZone)
{
    const std::string zone1 = "zone1";
    const std::string zone2 = "zone2";

    ZonesManager cm(TEST_CONFIG_PATH);
    cm.createZone(zone1, DBUS_TEMPLATE);
    cm.createZone(zone2, DBUS_TEMPLATE);

    Latch callDone;
    auto resultCallback = [&]() {
        callDone.set();
    };

    HostAccessory host;

    // start zone1
    host.callAsyncMethodStartZone(zone1, resultCallback);
    BOOST_REQUIRE(callDone.wait(EVENT_TIMEOUT));
    BOOST_CHECK(cm.isRunning(zone1));
    BOOST_CHECK_EQUAL(cm.getRunningForegroundZoneId(), zone1);

    // start zone2
    host.callAsyncMethodStartZone(zone2, resultCallback);
    BOOST_REQUIRE(callDone.wait(EVENT_TIMEOUT));
    BOOST_CHECK(cm.isRunning(zone2));
    BOOST_CHECK_EQUAL(cm.getRunningForegroundZoneId(), zone2);

    // shutdown zone2
    host.callAsyncMethodShutdownZone(zone2, resultCallback);
    BOOST_REQUIRE(callDone.wait(EVENT_TIMEOUT));
    BOOST_CHECK(!cm.isRunning(zone2));

    // shutdown zone1
    host.callAsyncMethodShutdownZone(zone1, resultCallback);
    BOOST_REQUIRE(callDone.wait(EVENT_TIMEOUT));
    BOOST_CHECK(!cm.isRunning(zone1));
    BOOST_CHECK_EQUAL(cm.getRunningForegroundZoneId(), "");
}

BOOST_AUTO_TEST_CASE(LockUnlockZone)
{
    ZonesManager cm(TEST_CONFIG_PATH);
    cm.createZone("zone1", DBUS_TEMPLATE);
    cm.createZone("zone2", DBUS_TEMPLATE);
    cm.createZone("zone3", DBUS_TEMPLATE);
    cm.restoreAll();

    HostAccessory host;

    std::vector<std::string> zoneIds = {"zone1",
                                        "zone2",
                                        "zone3"};

    for (const std::string& zoneId: zoneIds){
        try {
            host.callMethodLockZone(zoneId);
        } catch(const std::exception&) {
            //This try catch clause is for prevent from test crashing
            //and should be removed after resolve following errors
            //TODO: Abort when zone is locked on destroying ZonesManager
            HostAccessory host2; //TODO: After IPCTimeoutException host is useless -- fix it
            try { host2.callMethodUnlockZone(zoneId); } catch (...) {};
            throw;
        }
        BOOST_CHECK(cm.isPaused(zoneId));
        host.callMethodUnlockZone(zoneId);
        BOOST_CHECK(cm.isRunning(zoneId));
    }

    BOOST_REQUIRE_EXCEPTION(host.callMethodLockZone(NON_EXISTANT_ZONE_ID),
                            std::exception, //TODO: exception should be more specific
                            WhatEquals("No such zone id"));
    BOOST_REQUIRE_EXCEPTION(host.callMethodUnlockZone(NON_EXISTANT_ZONE_ID),
                            std::exception, //TODO: exception should be more specific
                            WhatEquals("No such zone id"));

    cm.shutdownAll();
    BOOST_REQUIRE_EXCEPTION(host.callMethodLockZone("zone1"),
                            std::exception, //TODO: exception should be more specific
                            WhatEquals("Zone is not running"));
    BOOST_REQUIRE_EXCEPTION(host.callMethodUnlockZone("zone1"),
                            std::exception, //TODO: exception should be more specific
                            WhatEquals("Zone is not paused"));
}

BOOST_AUTO_TEST_SUITE_END()
