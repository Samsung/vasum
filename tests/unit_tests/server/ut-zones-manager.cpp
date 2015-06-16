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
#ifdef DBUS_CONNECTION
// TODO: Switch to real power-manager dbus defs when they will be implemented in power-manager
#include "fake-power-manager-dbus-definitions.hpp"
#include "host-dbus-definitions.hpp"
#include "test-dbus-definitions.hpp"
#include "dbus/connection.hpp"
#include "dbus/exception.hpp"
#endif //DBUS_CONNECTION
#include "host-ipc-definitions.hpp"
#include <api/messages.hpp>
#include <ipc/epoll/thread-dispatcher.hpp>
#include <ipc/client.hpp>
#include "exception.hpp"

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
#include <fcntl.h>
#include <sys/stat.h>

using namespace vasum;
using namespace config;
using namespace utils;
#ifdef DBUS_CONNECTION
using namespace dbus;
#endif //DBUS_CONNECTION


namespace {

const std::string CONFIG_DIR = VSM_TEST_CONFIG_INSTALL_DIR;
const std::string TEST_CONFIG_PATH = CONFIG_DIR + "/test-daemon.conf";
const std::string MISSING_CONFIG_PATH = CONFIG_DIR + "/missing-daemon.conf";
const int EVENT_TIMEOUT = 5000;
const int SIGNAL_PROPAGATE_TIME = 500; // ms
const std::int32_t DEFAULT_FILE_MODE = 0666;
//const int UNEXPECTED_EVENT_TIMEOUT = EVENT_TIMEOUT / 5;
const std::string TEST_APP_NAME = "testapp";
const std::string TEST_MESSAGE = "testmessage";
const std::string FILE_CONTENT = "File content\n"
                                 "Line 1\n"
                                 "Line 2\n";
const std::string NON_EXISTANT_ZONE_ID = "NON_EXISTANT_ZONE_ID";
const std::string ZONES_PATH = "/tmp/ut-zones"; // the same as in daemon.conf
const std::string SIMPLE_TEMPLATE = "console-ipc";

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

class HostDbusAccessory {
public:
    static const int HOST_ID = 0;

    typedef std::function<void(const std::string& argument,
                               MethodResultBuilder::Pointer result
                              )> TestApiMethodCallback;
    typedef std::function<void()> VoidResultCallback;
    typedef std::function<void(const api::Notification)> NotificationCallback;
    typedef std::map<std::string, std::string> Connections;

    HostDbusAccessory()
        : mId(0),
          mClient(DbusConnection::create(acquireAddress())),
          mNameAcquired(false),
          mPendingDisconnect(false)
    {
    }

    HostDbusAccessory(int id)
        : mId(id),
          mClient(DbusConnection::create(acquireAddress())),
          mNameAcquired(false),
          mPendingDisconnect(false)
    {
    }

    void setName(const std::string& name)
    {
        mClient->setName(name,
                         std::bind(&HostDbusAccessory::onNameAcquired, this),
                         std::bind(&HostDbusAccessory::onDisconnect, this));

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
        mClient->signalSubscribe(callback, isHost() ? api::dbus::BUS_NAME : api::dbus::BUS_NAME);
    }

    void subscribeNotification(const NotificationCallback& callback)
    {
        auto handler = [callback](const std::string& /*senderBusName*/,
                          const std::string& objectPath,
                          const std::string& interface,
                          const std::string& signalName,
                           GVariant* parameters)
        {
            if (objectPath == api::dbus::OBJECT_PATH &&
                interface == api::dbus::INTERFACE &&
                signalName == api::dbus::SIGNAL_NOTIFICATION &&
                g_variant_is_of_type(parameters, G_VARIANT_TYPE("(sss)"))) {

                const gchar* zone = NULL;
                const gchar* application = NULL;
                const gchar* message = NULL;
                g_variant_get(parameters, "(&s&s&s)", &zone, &application, &message);
                callback({zone, application, message});
            }
        };
        mClient->signalSubscribe(handler, api::dbus::BUS_NAME);
    }

    void signalSwitchToDefault()
    {
        // emit signal from dbus connection
        mClient->emitSignal(api::dbus::OBJECT_PATH,
                            api::dbus::INTERFACE,
                            api::dbus::SIGNAL_SWITCH_TO_DEFAULT,
                            nullptr);
    }

    void callMethodNotify()
    {
        GVariant* parameters = g_variant_new("(ss)", TEST_APP_NAME.c_str(), TEST_MESSAGE.c_str());
        mClient->callMethod(api::dbus::BUS_NAME,
                            api::dbus::OBJECT_PATH,
                            api::dbus::INTERFACE,
                            api::dbus::METHOD_NOTIFY_ACTIVE_ZONE,
                            parameters,
                            "()");
    }

    std::string callMethodMove(const std::string& dest, const std::string& path)
    {
        GVariant* parameters = g_variant_new("(ss)", dest.c_str(), path.c_str());
        GVariantPtr result = mClient->callMethod(api::dbus::BUS_NAME,
                                                 api::dbus::OBJECT_PATH,
                                                 api::dbus::INTERFACE,
                                                 api::dbus::METHOD_FILE_MOVE_REQUEST,
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
        GVariantPtr result = mClient->callMethod(isHost() ? api::dbus::BUS_NAME :
                                                            api::dbus::BUS_NAME,
                                                 isHost() ? api::dbus::OBJECT_PATH :
                                                            api::dbus::OBJECT_PATH,
                                                 isHost() ? api::dbus::INTERFACE :
                                                            api::dbus::INTERFACE,
                                                 api::dbus::METHOD_PROXY_CALL,
                                                 packedParameters,
                                                 "(v)");
        GVariant* unpackedResult = NULL;
        g_variant_get(result.get(), "(v)", &unpackedResult);
        return GVariantPtr(unpackedResult, g_variant_unref);
    }

    std::vector<std::string> callMethodGetZoneIds()
    {
        assert(isHost());
        GVariantPtr result = mClient->callMethod(api::dbus::BUS_NAME,
                                                 api::dbus::OBJECT_PATH,
                                                 api::dbus::INTERFACE,
                                                 api::dbus::METHOD_GET_ZONE_ID_LIST,
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
        GVariantPtr result = mClient->callMethod(api::dbus::BUS_NAME,
                                                 api::dbus::OBJECT_PATH,
                                                 api::dbus::INTERFACE,
                                                 api::dbus::METHOD_GET_ACTIVE_ZONE_ID,
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
        GVariantPtr result = mClient->callMethod(api::dbus::BUS_NAME,
                                                 api::dbus::OBJECT_PATH,
                                                 api::dbus::INTERFACE,
                                                 api::dbus::METHOD_SET_ACTIVE_ZONE,
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
        mClient->callMethodAsync(api::dbus::BUS_NAME,
                                 api::dbus::OBJECT_PATH,
                                 api::dbus::INTERFACE,
                                 api::dbus::METHOD_CREATE_ZONE,
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
        mClient->callMethodAsync(api::dbus::BUS_NAME,
                                 api::dbus::OBJECT_PATH,
                                 api::dbus::INTERFACE,
                                 api::dbus::METHOD_DESTROY_ZONE,
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
        mClient->callMethodAsync(api::dbus::BUS_NAME,
                                 api::dbus::OBJECT_PATH,
                                 api::dbus::INTERFACE,
                                 api::dbus::METHOD_SHUTDOWN_ZONE,
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
        mClient->callMethodAsync(api::dbus::BUS_NAME,
                                 api::dbus::OBJECT_PATH,
                                 api::dbus::INTERFACE,
                                 api::dbus::METHOD_START_ZONE,
                                 parameters,
                                 "()",
                                 dropException(asyncResult));
    }

    void callMethodLockZone(const std::string& id)
    {
        assert(isHost());
        GVariant* parameters = g_variant_new("(s)", id.c_str());
        GVariantPtr result = mClient->callMethod(api::dbus::BUS_NAME,
                                                 api::dbus::OBJECT_PATH,
                                                 api::dbus::INTERFACE,
                                                 api::dbus::METHOD_LOCK_ZONE,
                                                 parameters,
                                                 "()");
    }

    void callMethodUnlockZone(const std::string& id)
    {
        assert(isHost());
        GVariant* parameters = g_variant_new("(s)", id.c_str());
        GVariantPtr result = mClient->callMethod(api::dbus::BUS_NAME,
                                                 api::dbus::OBJECT_PATH,
                                                 api::dbus::INTERFACE,
                                                 api::dbus::METHOD_UNLOCK_ZONE,
                                                 parameters,
                                                 "()");
    }

    int callMethodCreateFile(const std::string& id,
                             const std::string& path,
                             const std::int32_t& flags,
                             const std::int32_t& mode)
    {
        assert(isHost());
        GVariant* parameters = g_variant_new("(ssii)", id.c_str(), path.c_str(), flags, mode);
        GVariantPtr result = mClient->callMethod(api::dbus::BUS_NAME,
                                                 api::dbus::OBJECT_PATH,
                                                 api::dbus::INTERFACE,
                                                 api::dbus::METHOD_CREATE_FILE,
                                                 parameters,
                                                 "(h)");
        int fileFd = 0;
        g_variant_get(result.get(), "(h)", &fileFd);
        return fileFd;
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

#endif //DBUS_CONNECTION

class HostIPCAccessory {
public:
    typedef std::function<void()> VoidResultCallback;
    typedef std::function<void(const api::Notification)> NotificationCallback;

    HostIPCAccessory()
        : mClient(mDispatcher.getPoll(), HOST_IPC_SOCKET)
    {
        mClient.start();
    }

    std::vector<std::string> callMethodGetZoneIds()
    {
        const auto out = mClient.callSync<api::Void, api::ZoneIds>(api::ipc::METHOD_GET_ZONE_ID_LIST,
                                                                    std::make_shared<api::Void>());
        return out->values;
    }

    std::string callMethodGetActiveZoneId()
    {
        const auto out = mClient.callSync<api::Void, api::ZoneId>(api::ipc::METHOD_GET_ACTIVE_ZONE_ID,
                                                                  std::make_shared<api::Void>());
        return out->value;
    }

    void callMethodSetActiveZone(const std::string& id)
    {
        mClient.callSync<api::ZoneId, api::Void>(api::ipc::METHOD_SET_ACTIVE_ZONE,
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
        mClient.callAsync<api::CreateZoneIn, api::Void>(api::ipc::METHOD_CREATE_ZONE,
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
        mClient.callAsync<api::ZoneId, api::Void>(api::ipc::METHOD_DESTROY_ZONE,
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
        mClient.callAsync<api::ZoneId, api::Void>(api::ipc::METHOD_SHUTDOWN_ZONE,
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
        mClient.callAsync<api::ZoneId, api::Void>(api::ipc::METHOD_START_ZONE,
                           std::make_shared<api::ZoneId>(api::ZoneId{id}),
                           asyncResult);
    }

    void callMethodLockZone(const std::string& id)
    {
        mClient.callSync<api::ZoneId, api::Void>(api::ipc::METHOD_LOCK_ZONE,
                                                  std::make_shared<api::ZoneId>(api::ZoneId{id}),
                                                  EVENT_TIMEOUT*10); //Prevent from IPCTimeoutException see LockUnlockZone
    }

    void callMethodUnlockZone(const std::string& id)
    {
        mClient.callSync<api::ZoneId, api::Void>(api::ipc::METHOD_UNLOCK_ZONE,
                                                  std::make_shared<api::ZoneId>(api::ZoneId{id}),
                                                  EVENT_TIMEOUT*10); //Prevent from IPCTimeoutException see LockUnlockZone
    }

    void subscribeNotification(const NotificationCallback& callback)
    {
        auto callbackWrapper = [callback] (const ipc::PeerID, std::shared_ptr<api::Notification>& data) {
            callback(*data);
        };
        mClient.setSignalHandler<api::Notification>(api::ipc::SIGNAL_NOTIFICATION,
                                                    callbackWrapper);
    }

    void signalSwitchToDefault()
    {
        mClient.signal<api::Void>(api::ipc::SIGNAL_SWITCH_TO_DEFAULT, std::make_shared<api::Void>());
    }

    void callMethodNotify()
    {
        mClient.callSync<api::NotifActiveZoneIn, api::Void>(
            api::ipc::METHOD_NOTIFY_ACTIVE_ZONE,
            std::make_shared<api::NotifActiveZoneIn>(api::NotifActiveZoneIn{TEST_APP_NAME, TEST_MESSAGE}),
            EVENT_TIMEOUT*10); //Prevent from IPCTimeoutException see LockUnlockZone
    }

    std::string callMethodMove(const std::string& dest, const std::string& path)
    {
        auto result = mClient.callSync<api::FileMoveRequestIn, api::FileMoveRequestStatus>(
            api::ipc::METHOD_FILE_MOVE_REQUEST,
            std::make_shared<api::FileMoveRequestIn>(api::FileMoveRequestIn{dest, path}),
            EVENT_TIMEOUT*10);
        return result->value;
    }

    int callMethodCreateFile(const std::string& id,
                             const std::string& path,
                             const std::int32_t& flags,
                             const std::int32_t& mode)
    {
        auto result = mClient.callSync<api::CreateFileIn, api::CreateFileOut>(
            api::ipc::METHOD_CREATE_FILE,
            std::make_shared<api::CreateFileIn>(api::CreateFileIn{id, path, flags, mode}),
            EVENT_TIMEOUT*10);
        return result->fd.value;
    }

private:
    ipc::epoll::ThreadDispatcher mDispatcher;
    ipc::Client mClient;
};

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

} // namespace

struct Fixture {
    ScopedGlibLoop mLoop;

    ScopedDir mZonesPathGuard;
    ScopedDir mRunGuard;

    Fixture()
        : mZonesPathGuard(ZONES_PATH)
        , mRunGuard("/tmp/ut-run")
    {}
};

struct IPCFixture : Fixture {
    typedef HostIPCAccessory HostAccessory;
};

#ifdef DBUS_CONNECTION
struct DbusFixture : Fixture {
    typedef HostDbusAccessory HostAccessory;
};
#endif //DBUS_CONNECTION

#ifdef DBUS_CONNECTION
#define ACCESSORS  IPCFixture, DbusFixture
#else //DBUS_CONNECTION
#define ACCESSORS  IPCFixture
#endif //DBUS_CONNECTION


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

#ifdef ZONE_CONNECTION
BOOST_AUTO_TEST_CASE(NotifyActiveZone)
{
    ZonesManager cm(TEST_CONFIG_PATH);
    cm.createZone("zone1", SIMPLE_TEMPLATE);
    cm.createZone("zone2", SIMPLE_TEMPLATE);
    cm.createZone("zone3", SIMPLE_TEMPLATE);
    cm.restoreAll();

    Latch signalReceivedLatch;
    std::map<int, std::vector<std::string>> signalReceivedSourcesMap;

    std::map<int, std::unique_ptr<ZoneAccessory>> connections;
    for (int i = 1; i <= TEST_DBUS_CONNECTION_ZONES_COUNT; ++i) {
        connections[i] = std::unique_ptr<ZoneAccessory>(new ZoneAccessory(i));
    }

    auto handler = [](Latch& latch,
                      std::vector<std::string>& receivedSignalSources,
                      const api::Notification& notify)
        {
            receivedSignalSources.push_back(notify.zone);
            if (notify.application == TEST_APP_NAME && notify.message == TEST_MESSAGE) {
                latch.set();
            }
        };

    using namespace std::placeholders;
    for (int i = 1; i <= TEST_DBUS_CONNECTION_ZONES_COUNT; ++i) {
        connections[i]->subscribeNotification(std::bind(handler,
                                              std::ref(signalReceivedLatch),
                                              std::ref(signalReceivedSourcesMap[i]),
                                              _1));
    }
    for (auto& connection : connections) {
        connection.second->callMethodNotify();
    }

    BOOST_REQUIRE(signalReceivedLatch.waitForN(connections.size() - 1u, EVENT_TIMEOUT));
    BOOST_REQUIRE(signalReceivedLatch.empty());

    //check if there are no signals that was received more than once
    for (const auto& source : signalReceivedSourcesMap[1]) {
        BOOST_CHECK_EQUAL(std::count(signalReceivedSourcesMap[1].begin(),
                                     signalReceivedSourcesMap[1].end(),
                                     source), 1);
    }
    //check if all signals was received by active zone
    BOOST_CHECK_EQUAL(signalReceivedSourcesMap[1].size(), connections.size() - 1);
    //check if no signals was received by inactive zone
    for (size_t i = 2; i <= connections.size(); ++i) {
        BOOST_CHECK(signalReceivedSourcesMap[i].empty());
    }

    connections.clear();
}

BOOST_AUTO_TEST_CASE(MoveFile)
{
    ZonesManager cm(TEST_CONFIG_PATH);
    cm.createZone("zone1", SIMPLE_TEMPLATE);
    cm.createZone("zone2", SIMPLE_TEMPLATE);
    cm.createZone("zone3", SIMPLE_TEMPLATE);
    cm.restoreAll();

    Latch notificationLatch;
    std::string notificationSource;
    std::string notificationPath;
    std::string notificationRetcode;

    std::map<int, std::unique_ptr<ZoneAccessory>> connections;
    for (int i = 1; i <= 2; ++i) {
        connections[i] = std::unique_ptr<ZoneAccessory>(new ZoneAccessory(i));
    }

    auto handler = [&](const api::Notification& notify)
        {
                notificationSource = notify.zone;
                notificationPath = notify.application;
                notificationRetcode = notify.message;
                notificationLatch.set();
        };

    // subscribe the second (destination) zone for notifications
    connections.at(2)->subscribeNotification(handler);

    const std::string TMP = "/tmp/ut-zones";
    const std::string NO_PATH = "path_doesnt_matter_here";
    const std::string BUGGY_PATH = TMP + "/this_file_does_not_exist";
    const std::string BUGGY_ZONE = "this-zone-does-not-exist";
    const std::string ZONE1 = "zone1";
    const std::string ZONE2 = "zone2";
    const std::string ZONE1PATH = TMP + "/" + ZONE1 + TMP;
    const std::string ZONE2PATH = TMP + "/" + ZONE2 + TMP;

    // sending to a non existing zone
    BOOST_CHECK_EQUAL(connections.at(1)->callMethodMove(BUGGY_ZONE, NO_PATH),
                      api::FILE_MOVE_DESTINATION_NOT_FOUND);
    BOOST_CHECK(notificationLatch.empty());

    // sending to self
    BOOST_CHECK_EQUAL(connections.at(1)->callMethodMove(ZONE1, NO_PATH),
                      api::FILE_MOVE_WRONG_DESTINATION);
    BOOST_CHECK(notificationLatch.empty());

    // no permission to send
    BOOST_CHECK_EQUAL(connections.at(1)->callMethodMove(ZONE2, "/etc/secret1"),
                      api::FILE_MOVE_NO_PERMISSIONS_SEND);
    BOOST_CHECK(notificationLatch.empty());

    // no permission to receive
    // TODO uncomment this after adding an api to change 'permittedTo*' config
    //BOOST_CHECK_EQUAL(connections.at(1)->callMethodMove(ZONE2, "/etc/secret2"),
    //                  api::FILE_MOVE_NO_PERMISSIONS_RECEIVE);
    //BOOST_CHECK(notificationLatch.empty());

    // non existing file
    BOOST_CHECK_EQUAL(connections.at(1)->callMethodMove(ZONE2, BUGGY_PATH),
                      api::FILE_MOVE_FAILED);
    BOOST_CHECK(notificationLatch.empty());

    // a working scenario
    namespace fs = boost::filesystem;
    boost::system::error_code ec;
    fs::remove_all(ZONE1PATH, ec);
    fs::remove_all(ZONE2PATH, ec);
    BOOST_REQUIRE(fs::create_directories(ZONE1PATH, ec));
    BOOST_REQUIRE(fs::create_directories(ZONE2PATH, ec));
    BOOST_REQUIRE(utils::saveFileContent(ZONE1PATH + "/file", FILE_CONTENT));

    BOOST_CHECK_EQUAL(connections.at(1)->callMethodMove(ZONE2, TMP + "/file"),
                      api::FILE_MOVE_SUCCEEDED);
    BOOST_REQUIRE(notificationLatch.wait(EVENT_TIMEOUT));
    BOOST_REQUIRE(notificationLatch.empty());
    BOOST_CHECK_EQUAL(notificationSource, ZONE1);
    BOOST_CHECK_EQUAL(notificationPath, TMP + "/file");
    BOOST_CHECK_EQUAL(notificationRetcode, api::FILE_MOVE_SUCCEEDED);
    BOOST_CHECK(!fs::exists(ZONE1PATH + "/file"));
    BOOST_CHECK_EQUAL(utils::readFileContent(ZONE2PATH + "/file"), FILE_CONTENT);

    fs::remove_all(ZONE1PATH, ec);
    fs::remove_all(ZONE2PATH, ec);
}
#endif

MULTI_FIXTURE_TEST_CASE(SwitchToDefault, F, ACCESSORS)
{
    ZonesManager cm(TEST_CONFIG_PATH);
    cm.createZone("zone1", SIMPLE_TEMPLATE);
    cm.createZone("zone2", SIMPLE_TEMPLATE);
    cm.createZone("zone3", SIMPLE_TEMPLATE);
    cm.restoreAll();

    typename F::HostAccessory host;
    std::this_thread::sleep_for(std::chrono::milliseconds(SIGNAL_PROPAGATE_TIME));

    auto isDefaultFocused = [&cm]() -> bool {
        return cm.getRunningForegroundZoneId() == "zone1";
    };

    cm.focus("zone3");

    host.signalSwitchToDefault();

    // check if default zone has focus
    BOOST_CHECK(spinWaitFor(EVENT_TIMEOUT, isDefaultFocused));
}

MULTI_FIXTURE_TEST_CASE(AllowSwitchToDefault, F, ACCESSORS)
{
    ZonesManager cm(TEST_CONFIG_PATH);
    cm.createZone("zone1", SIMPLE_TEMPLATE);
    cm.createZone("zone2", SIMPLE_TEMPLATE);
    cm.createZone("zone3", SIMPLE_TEMPLATE);
    cm.restoreAll();

    typename F::HostAccessory host;
    std::this_thread::sleep_for(std::chrono::milliseconds(SIGNAL_PROPAGATE_TIME));

    auto isDefaultFocused = [&cm]() -> bool {
        return cm.getRunningForegroundZoneId() == "zone1";
    };

    // focus non-default zone with allowed switching
    cm.focus("zone3");

    host.signalSwitchToDefault();

    // check if default zone has focus
    BOOST_CHECK(spinWaitFor(EVENT_TIMEOUT, isDefaultFocused));

    // focus non-default zone with disabled switching
    cm.focus("zone2");

    host.signalSwitchToDefault();

    // now default zone should not be focused
    // TODO uncomment this after adding an api to change 'switchToDefaultAfterTimeout'
    //BOOST_CHECK(!spinWaitFor(UNEXPECTED_EVENT_TIMEOUT, isDefaultFocused));
}

#ifdef DBUS_CONNECTION
MULTI_FIXTURE_TEST_CASE(ProxyCall, F, DbusFixture)
{
    ZonesManager cm(TEST_CONFIG_PATH);
    cm.createZone("zone1", SIMPLE_TEMPLATE);
    cm.createZone("zone2", SIMPLE_TEMPLATE);
    cm.createZone("zone3", SIMPLE_TEMPLATE);
    cm.restoreAll();

    typename F::HostAccessory host;
    host.setName(testapi::BUS_NAME);

    auto handler = [](const std::string& argument, MethodResultBuilder::Pointer result) {
        if (argument.empty()) {
            result->setError("org.tizen.vasum.Error.Test", "Test error");
        } else {
            std::string ret = "reply from host: " + argument;
            result->set(g_variant_new("(s)", ret.c_str()));
        }
    };
    host.registerTestApiObject(handler);

    // host -> host
    BOOST_CHECK_EQUAL("reply from host: param2",
                      host.testApiProxyCall("host", "param2"));

    // host -> unknown
    BOOST_CHECK_EXCEPTION(host.testApiProxyCall("unknown", "param"),
                          DbusCustomException,
                          WhatEquals("Unknown proxy call target"));

    // forwarding error
    BOOST_CHECK_EXCEPTION(host.testApiProxyCall("host", ""),
                          DbusCustomException,
                          WhatEquals("Test error"));

    // forbidden call
    BOOST_CHECK_EXCEPTION(host.proxyCall("host",
                                         "org.fake",
                                         "/a/b",
                                         "c.d",
                                         "foo",
                                         g_variant_new("(s)", "arg")),
                          DbusCustomException,
                          WhatEquals("Proxy call forbidden"));
}
#endif //DBUS_CONNECTION

namespace {

const std::set<std::string> EXPECTED_CONNECTIONS_NONE = { "zone1", "zone2", "zone3" };

} // namespace

MULTI_FIXTURE_TEST_CASE(GetZoneIds, F, ACCESSORS)
{
    ZonesManager cm(TEST_CONFIG_PATH);
    cm.createZone("zone1", SIMPLE_TEMPLATE);
    cm.createZone("zone2", SIMPLE_TEMPLATE);
    cm.createZone("zone3", SIMPLE_TEMPLATE);

    typename F::HostAccessory host;

    std::vector<std::string> zoneIds = {"zone1",
                                        "zone2",
                                        "zone3"};
    std::vector<std::string> returnedIds = host.callMethodGetZoneIds();

    BOOST_CHECK(returnedIds == zoneIds);// order should be preserved
}

MULTI_FIXTURE_TEST_CASE(GetActiveZoneId, F, ACCESSORS)
{
    ZonesManager cm(TEST_CONFIG_PATH);
    cm.createZone("zone1", SIMPLE_TEMPLATE);
    cm.createZone("zone2", SIMPLE_TEMPLATE);
    cm.createZone("zone3", SIMPLE_TEMPLATE);
    cm.restoreAll();

    typename F::HostAccessory host;

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

MULTI_FIXTURE_TEST_CASE(SetActiveZone, F, ACCESSORS)
{
    ZonesManager cm(TEST_CONFIG_PATH);
    cm.createZone("zone1", SIMPLE_TEMPLATE);
    cm.createZone("zone2", SIMPLE_TEMPLATE);
    cm.createZone("zone3", SIMPLE_TEMPLATE);
    cm.restoreAll();

    typename F::HostAccessory host;

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

MULTI_FIXTURE_TEST_CASE(CreateDestroyZone, F, ACCESSORS)
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

    typename F::HostAccessory host;

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

MULTI_FIXTURE_TEST_CASE(CreateDestroyZonePersistence, F, ACCESSORS)
{
    const std::string zone = "test1";

    Latch callDone;
    auto resultCallback = [&]() {
        callDone.set();
    };

    auto getZoneIds = []() -> std::vector<std::string> {
        ZonesManager cm(TEST_CONFIG_PATH);
        cm.restoreAll();

        typename F::HostAccessory host;
        return host.callMethodGetZoneIds();
    };

    BOOST_CHECK(getZoneIds().empty());

    // create zone
    {
        ZonesManager cm(TEST_CONFIG_PATH);
        typename F::HostAccessory host;
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
        typename F::HostAccessory host;
        host.callAsyncMethodDestroyZone(zone, resultCallback);
        BOOST_REQUIRE(callDone.wait(EVENT_TIMEOUT));
    }

    BOOST_CHECK(getZoneIds().empty());
}

MULTI_FIXTURE_TEST_CASE(ZoneStatePersistence, F, ACCESSORS)
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
        typename F::HostAccessory host;

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

MULTI_FIXTURE_TEST_CASE(StartShutdownZone, F, ACCESSORS)
{
    const std::string zone1 = "zone1";
    const std::string zone2 = "zone2";

    ZonesManager cm(TEST_CONFIG_PATH);
    cm.createZone(zone1, SIMPLE_TEMPLATE);
    cm.createZone(zone2, SIMPLE_TEMPLATE);

    Latch callDone;
    auto resultCallback = [&]() {
        callDone.set();
    };

    typename F::HostAccessory host;

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

MULTI_FIXTURE_TEST_CASE(LockUnlockZone, F, ACCESSORS)
{
    ZonesManager cm(TEST_CONFIG_PATH);
    cm.createZone("zone1", SIMPLE_TEMPLATE);
    cm.createZone("zone2", SIMPLE_TEMPLATE);
    cm.createZone("zone3", SIMPLE_TEMPLATE);
    cm.restoreAll();

    typename F::HostAccessory host;

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
            typename F::HostAccessory host2; //TODO: After IPCTimeoutException host is useless -- fix it
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

MULTI_FIXTURE_TEST_CASE(CreateFile, F, ACCESSORS)
{
    ZonesManager cm(TEST_CONFIG_PATH);
    cm.createZone("zone1", SIMPLE_TEMPLATE);
    cm.restoreAll();

    typename F::HostAccessory host;
    int returnedFd = host.callMethodCreateFile("zone1", "/123.txt", O_RDWR, DEFAULT_FILE_MODE);
    BOOST_REQUIRE(::fcntl(returnedFd, F_GETFD) != -1);
    BOOST_REQUIRE(::close(returnedFd) != -1);

    returnedFd = host.callMethodCreateFile("zone1", "/56.txt", O_RDONLY, DEFAULT_FILE_MODE);
    BOOST_REQUIRE(::fcntl(returnedFd, F_GETFD) != -1);
    BOOST_REQUIRE(::close(returnedFd) != -1);

    returnedFd = host.callMethodCreateFile("zone1", "/89.txt", O_WRONLY, DEFAULT_FILE_MODE);
    BOOST_REQUIRE(::fcntl(returnedFd, F_GETFD) != -1);
    BOOST_REQUIRE(::close(returnedFd) != -1);
}

MULTI_FIXTURE_TEST_CASE(CreateWriteReadFile, F, ACCESSORS)
{
    ZonesManager cm(TEST_CONFIG_PATH);
    cm.createZone("zone1", SIMPLE_TEMPLATE);
    cm.restoreAll();

    typename F::HostAccessory host;

    // create file, make sure returned fd is correct
    int returnedFd = host.callMethodCreateFile("zone1", "/test123.txt", O_RDWR, DEFAULT_FILE_MODE);
    BOOST_REQUIRE(::fcntl(returnedFd, F_GETFD) != -1);

    // write sth
    BOOST_REQUIRE_MESSAGE(::write(returnedFd, FILE_CONTENT.c_str(), FILE_CONTENT.size()) > 0,
                          "Failed to write to fd " << returnedFd << ": " << ::strerror(errno));

    // go back to the beginning
    BOOST_REQUIRE(::lseek(returnedFd, 0, SEEK_SET) != -1);

    // read it back, zero-terminate it and verify
    std::unique_ptr<char[]> retStr(new char[FILE_CONTENT.size()+1]);
    BOOST_REQUIRE_MESSAGE(::read(returnedFd, retStr.get(), FILE_CONTENT.size()) > 0,
                          "Failed to read from file: " << ::strerror(errno));
    retStr[FILE_CONTENT.size()] = 0;
    BOOST_REQUIRE(FILE_CONTENT.compare(retStr.get()) == 0);

    // close
    BOOST_REQUIRE(::close(returnedFd) != -1);
}

BOOST_AUTO_TEST_SUITE_END()
