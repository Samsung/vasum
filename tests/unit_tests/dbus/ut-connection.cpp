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
 * @brief   Dbus connection unit tests
 */

#ifdef DBUS_CONNECTION
#include "config.hpp"
#include "ut.hpp"
#include "dbus/test-server.hpp"
#include "dbus/test-client.hpp"
#include "dbus/test-common.hpp"
#include "utils/scoped-daemon.hpp"
#include "utils/scoped-dir.hpp"

#include "dbus/connection.hpp"
#include "dbus/exception.hpp"
#include "utils/glib-loop.hpp"
#include "utils/file-wait.hpp"
#include "utils/latch.hpp"
#include "utils/fs.hpp"
#include "logger/logger.hpp"

#include <thread>
#include <mutex>
#include <condition_variable>

//TODO: BOOST_* macros aren't thread-safe. Remove it from callbacks

BOOST_AUTO_TEST_SUITE(DbusSuite)

using namespace utils;
using namespace vasum;
using namespace dbus;

namespace {

const char* DBUS_DAEMON_PROC = "/bin/dbus-daemon";
const char* const DBUS_DAEMON_ARGS[] = {
    DBUS_DAEMON_PROC,
    "--config-file=" VSM_TEST_CONFIG_INSTALL_DIR "/dbus/ut-dbus.conf",
    "--nofork",
    NULL
};
const int DBUS_DAEMON_TIMEOUT = 1000;
const int EVENT_TIMEOUT = 1000;

class ScopedDbusDaemon {
public:
    ScopedDbusDaemon()
        : mTestPathGuard(DBUS_SOCKET_DIR)
    {
        mDaemon.start(DBUS_DAEMON_PROC, DBUS_DAEMON_ARGS);
        waitForFile(DBUS_SOCKET_PATH, DBUS_DAEMON_TIMEOUT);
    }
    void stop()
    {
        mDaemon.stop();
    }
private:
    ScopedDir mTestPathGuard;
    ScopedDaemon mDaemon;
};

std::string getInterfaceFromIntrospectionXML(const std::string& xml, const std::string& name)
{
    std::string ret;
    GDBusNodeInfo* nodeInfo = g_dbus_node_info_new_for_xml(xml.c_str(), NULL);
    GDBusInterfaceInfo* iface = g_dbus_node_info_lookup_interface(nodeInfo, name.c_str());
    if (iface) {
        GString* gret = g_string_new("");
        g_dbus_interface_info_generate_xml(iface, 0, gret);
        ret.assign(gret->str, gret->len);
        g_string_free(gret, TRUE);
    }
    g_dbus_node_info_unref(nodeInfo);
    return ret;
}

} // namespace

BOOST_AUTO_TEST_CASE(DbusDaemon)
{
    ScopedDbusDaemon daemon;
}

BOOST_AUTO_TEST_CASE(NoDbus)
{
    ScopedGlibLoop loop;
    BOOST_CHECK_THROW(DbusConnection::create(DBUS_ADDRESS), DbusIOException);
}

BOOST_AUTO_TEST_CASE(Connection)
{
    ScopedGlibLoop loop;
    DbusConnection::Pointer connSystem = DbusConnection::createSystem();
}

BOOST_AUTO_TEST_CASE(Simple)
{
    ScopedDbusDaemon daemon;
    ScopedGlibLoop loop;
    Latch nameAcquired, nameLost;

    DbusConnection::Pointer conn1 = DbusConnection::create(DBUS_ADDRESS);
    DbusConnection::Pointer conn2 = DbusConnection::create(DBUS_ADDRESS);
    conn1->setName(TESTAPI_BUS_NAME,
                   [&] {nameAcquired.set();},
                   [&] {nameLost.set();});
    DbusConnection::Pointer connSystem = DbusConnection::createSystem();
    BOOST_CHECK(nameAcquired.wait(EVENT_TIMEOUT));
    BOOST_CHECK(nameLost.empty());
}

BOOST_AUTO_TEST_CASE(ConnectionLost)
{
    ScopedDbusDaemon daemon;
    ScopedGlibLoop loop;
    Latch nameAcquired, nameLost;

    DbusConnection::Pointer conn1 = DbusConnection::create(DBUS_ADDRESS);
    conn1->setName(TESTAPI_BUS_NAME,
                   [&] {nameAcquired.set();},
                   [&] {nameLost.set();});
    BOOST_CHECK(nameAcquired.wait(EVENT_TIMEOUT));
    BOOST_CHECK(nameLost.empty());

    // close dbus socket
    daemon.stop();
    BOOST_CHECK(nameLost.wait(EVENT_TIMEOUT));
}

BOOST_AUTO_TEST_CASE(NameOwner)
{
    ScopedDbusDaemon daemon;
    ScopedGlibLoop loop;
    Latch nameAcquired1, nameLost1;
    Latch nameAcquired2, nameLost2;

    DbusConnection::Pointer conn1 = DbusConnection::create(DBUS_ADDRESS);
    DbusConnection::Pointer conn2 = DbusConnection::create(DBUS_ADDRESS);

    // acquire name by conn1
    conn1->setName(TESTAPI_BUS_NAME,
                   [&] {nameAcquired1.set();},
                   [&] {nameLost1.set();});
    BOOST_CHECK(nameAcquired1.wait(EVENT_TIMEOUT));
    BOOST_CHECK(nameLost1.empty());

    // conn2 can't acquire name
    conn2->setName(TESTAPI_BUS_NAME,
                   [&] {nameAcquired2.set();},
                   [&] {nameLost2.set();});
    BOOST_CHECK(nameLost2.wait(EVENT_TIMEOUT));
    BOOST_CHECK(nameAcquired2.empty());

    // close conn1
    conn1.reset();
    // depending on dbus implementation conn2 can automatically acquire the name
    //BOOST_CHECK(nameAcquired2.wait(EVENT_TIMEOUT));
}

BOOST_AUTO_TEST_CASE(GenericSignal)
{
    ScopedDbusDaemon daemon;
    ScopedGlibLoop loop;
    Latch signalEmitted;

    DbusConnection::Pointer conn1 = DbusConnection::create(DBUS_ADDRESS);
    DbusConnection::Pointer conn2 = DbusConnection::create(DBUS_ADDRESS);

    const std::string OBJECT_PATH = "/a/b/c";
    const std::string INTERFACE = "a.b.c";
    const std::string SIGNAL_NAME = "Foo";

    auto handler = [&](const std::string& /*senderBusName*/,
                       const std::string& objectPath,
                       const std::string& interface,
                       const std::string& signalName,
                       GVariant* parameters) {
        if (objectPath == OBJECT_PATH &&
            interface == INTERFACE &&
            signalName == SIGNAL_NAME &&
            g_variant_is_of_type(parameters, G_VARIANT_TYPE_UNIT)) {
            signalEmitted.set();
        }
    };
    conn2->signalSubscribe(handler, std::string());

    conn1->emitSignal(OBJECT_PATH, INTERFACE, SIGNAL_NAME, NULL);
    BOOST_CHECK(signalEmitted.wait(EVENT_TIMEOUT));
}

BOOST_AUTO_TEST_CASE(FilteredSignal)
{
    ScopedDbusDaemon daemon;
    ScopedGlibLoop loop;
    Latch goodSignalEmitted;
    Latch wrongSignalEmitted;
    Latch nameAcquired;

    DbusConnection::Pointer conn1 = DbusConnection::create(DBUS_ADDRESS);
    DbusConnection::Pointer conn2 = DbusConnection::create(DBUS_ADDRESS);

    auto handler = [&](const std::string& /*senderBusName*/,
                       const std::string& objectPath,
                       const std::string& interface,
                       const std::string& signalName,
                       GVariant* parameters) {
        if (objectPath == TESTAPI_OBJECT_PATH &&
            interface == TESTAPI_INTERFACE &&
            signalName == TESTAPI_SIGNAL_NOTIFY &&
            g_variant_is_of_type(parameters, G_VARIANT_TYPE("(s)"))) {

            const gchar* msg = NULL;
            g_variant_get(parameters, "(&s)", &msg);
            if (msg == std::string("jipii")) {
                goodSignalEmitted.set();
            } else {
                wrongSignalEmitted.set();
            }
        }
    };
    conn2->signalSubscribe(handler, TESTAPI_BUS_NAME);

    conn1->emitSignal(TESTAPI_OBJECT_PATH,
                      TESTAPI_INTERFACE,
                      TESTAPI_SIGNAL_NOTIFY,
                      g_variant_new("(s)", "boo"));

    conn1->setName(TESTAPI_BUS_NAME,
                   [&] {nameAcquired.set();},
                   [] {});
    BOOST_REQUIRE(nameAcquired.wait(EVENT_TIMEOUT));

    conn1->emitSignal(TESTAPI_OBJECT_PATH,
                      TESTAPI_INTERFACE,
                      TESTAPI_SIGNAL_NOTIFY,
                      g_variant_new("(s)", "jipii"));

    BOOST_CHECK(goodSignalEmitted.wait(EVENT_TIMEOUT));
    BOOST_CHECK(wrongSignalEmitted.empty());
}

BOOST_AUTO_TEST_CASE(RegisterObject)
{
    ScopedDbusDaemon daemon;
    ScopedGlibLoop loop;
    DbusConnection::Pointer conn = DbusConnection::create(DBUS_ADDRESS);
    DbusConnection::MethodCallCallback callback;
    BOOST_CHECK_THROW(conn->registerObject(TESTAPI_OBJECT_PATH, "<invalid", callback, nullptr),
                      DbusInvalidArgumentException);
    BOOST_CHECK_THROW(conn->registerObject(TESTAPI_OBJECT_PATH, "", callback, nullptr),
                      DbusInvalidArgumentException);
    BOOST_CHECK_THROW(conn->registerObject(TESTAPI_OBJECT_PATH, "<node></node>", callback, nullptr),
                      DbusInvalidArgumentException);
    BOOST_CHECK_NO_THROW(conn->registerObject(TESTAPI_OBJECT_PATH, TESTAPI_DEFINITION, callback, nullptr));
}

BOOST_AUTO_TEST_CASE(IntrospectSystem)
{
    ScopedGlibLoop loop;
    DbusConnection::Pointer conn = DbusConnection::createSystem();
    std::string xml = conn->introspect("org.freedesktop.DBus", "/org/freedesktop/DBus");
    std::string iface = getInterfaceFromIntrospectionXML(xml, "org.freedesktop.DBus");
    BOOST_CHECK(!iface.empty());
}

BOOST_AUTO_TEST_CASE(Introspect)
{
    ScopedDbusDaemon daemon;
    ScopedGlibLoop loop;
    Latch nameAcquired;

    DbusConnection::Pointer conn1 = DbusConnection::create(DBUS_ADDRESS);
    DbusConnection::Pointer conn2 = DbusConnection::create(DBUS_ADDRESS);

    conn1->setName(TESTAPI_BUS_NAME,
                   [&] {nameAcquired.set();},
                   [] {});
    BOOST_REQUIRE(nameAcquired.wait(EVENT_TIMEOUT));
    conn1->registerObject(TESTAPI_OBJECT_PATH,
                          TESTAPI_DEFINITION,
                          DbusConnection::MethodCallCallback(),
                          nullptr);
    std::string xml = conn2->introspect(TESTAPI_BUS_NAME, TESTAPI_OBJECT_PATH);
    std::string iface = getInterfaceFromIntrospectionXML(xml, TESTAPI_INTERFACE);
    BOOST_REQUIRE(!iface.empty());
    BOOST_CHECK(std::string::npos != iface.find(TESTAPI_INTERFACE));
    BOOST_CHECK(std::string::npos != iface.find(TESTAPI_METHOD_NOOP));
    BOOST_CHECK(std::string::npos != iface.find(TESTAPI_METHOD_PROCESS));
    BOOST_CHECK(std::string::npos != iface.find(TESTAPI_METHOD_THROW));
    BOOST_CHECK(std::string::npos != iface.find(TESTAPI_SIGNAL_NOTIFY));
}

BOOST_AUTO_TEST_CASE(MethodCall)
{
    ScopedDbusDaemon daemon;
    ScopedGlibLoop loop;
    Latch nameAcquired;

    DbusConnection::Pointer conn1 = DbusConnection::create(DBUS_ADDRESS);
    DbusConnection::Pointer conn2 = DbusConnection::create(DBUS_ADDRESS);

    conn1->setName(TESTAPI_BUS_NAME,
                   [&] {nameAcquired.set();},
                   [] {});
    BOOST_REQUIRE(nameAcquired.wait(EVENT_TIMEOUT));
    auto handler = [](const std::string& objectPath,
                      const std::string& interface,
                      const std::string& methodName,
                      GVariant* parameters,
                      MethodResultBuilder::Pointer result) {
        if (objectPath != TESTAPI_OBJECT_PATH || interface != TESTAPI_INTERFACE) {
            return;
        }
        if (methodName == TESTAPI_METHOD_NOOP) {
            result->setVoid();
        } else if (methodName == TESTAPI_METHOD_PROCESS) {
            const gchar* arg = NULL;
            g_variant_get(parameters, "(&s)", &arg);
            std::string str = std::string("resp: ") + arg;
            result->set(g_variant_new("(s)", str.c_str()));
        } else if (methodName == TESTAPI_METHOD_THROW) {
            int arg = 0;
            g_variant_get(parameters, "(i)", &arg);
            result->setError("org.tizen.vasum.Error.Test", "msg: " + std::to_string(arg));
        }
    };
    conn1->registerObject(TESTAPI_OBJECT_PATH, TESTAPI_DEFINITION, handler, nullptr);

    GVariantPtr result1 = conn2->callMethod(TESTAPI_BUS_NAME,
                                            TESTAPI_OBJECT_PATH,
                                            TESTAPI_INTERFACE,
                                            TESTAPI_METHOD_NOOP,
                                            NULL,
                                            "()");
    BOOST_CHECK(g_variant_is_of_type(result1.get(), G_VARIANT_TYPE_UNIT));

    GVariantPtr result2 = conn2->callMethod(TESTAPI_BUS_NAME,
                                            TESTAPI_OBJECT_PATH,
                                            TESTAPI_INTERFACE,
                                            TESTAPI_METHOD_PROCESS,
                                            g_variant_new("(s)", "arg"),
                                            "(s)");
    const gchar* ret2 = NULL;
    g_variant_get(result2.get(), "(&s)", &ret2);
    BOOST_CHECK_EQUAL("resp: arg", ret2);

    BOOST_CHECK_THROW(conn2->callMethod(TESTAPI_BUS_NAME,
                                        TESTAPI_OBJECT_PATH,
                                        TESTAPI_INTERFACE,
                                        TESTAPI_METHOD_THROW,
                                        g_variant_new("(i)", 7),
                                        "()"),
                      DbusCustomException);
}

BOOST_AUTO_TEST_CASE(MethodAsyncCall)
{
    ScopedDbusDaemon daemon;
    ScopedGlibLoop loop;
    Latch nameAcquired;
    Latch callDone;

    DbusConnection::Pointer conn1 = DbusConnection::create(DBUS_ADDRESS);
    DbusConnection::Pointer conn2 = DbusConnection::create(DBUS_ADDRESS);

    conn1->setName(TESTAPI_BUS_NAME,
                   [&] {nameAcquired.set();},
                   [] {});
    BOOST_REQUIRE(nameAcquired.wait(EVENT_TIMEOUT));
    auto handler = [](const std::string& objectPath,
                      const std::string& interface,
                      const std::string& methodName,
                      GVariant* parameters,
                      MethodResultBuilder::Pointer result) {
        if (objectPath != TESTAPI_OBJECT_PATH || interface != TESTAPI_INTERFACE) {
            return;
        }
        if (methodName == TESTAPI_METHOD_NOOP) {
            result->setVoid();
        } else if (methodName == TESTAPI_METHOD_PROCESS) {
            const gchar* arg = NULL;
            g_variant_get(parameters, "(&s)", &arg);
            std::string str = std::string("resp: ") + arg;
            result->set(g_variant_new("(s)", str.c_str()));
        } else if (methodName == TESTAPI_METHOD_THROW) {
            int arg = 0;
            g_variant_get(parameters, "(i)", &arg);
            result->setError("org.tizen.vasum.Error.Test", "msg: " + std::to_string(arg));
        }
    };
    conn1->registerObject(TESTAPI_OBJECT_PATH, TESTAPI_DEFINITION, handler, nullptr);

    auto asyncResult1 = [&](dbus::AsyncMethodCallResult& asyncMethodCallResult) {
        if (g_variant_is_of_type(asyncMethodCallResult.get(), G_VARIANT_TYPE_UNIT)) {
            callDone.set();
        }
    };
    conn2->callMethodAsync(TESTAPI_BUS_NAME,
                           TESTAPI_OBJECT_PATH,
                           TESTAPI_INTERFACE,
                           TESTAPI_METHOD_NOOP,
                           NULL,
                           "()",
                           asyncResult1);
    BOOST_REQUIRE(callDone.wait(EVENT_TIMEOUT));

    auto asyncResult2 = [&](dbus::AsyncMethodCallResult& asyncMethodCallResult) {
        const gchar* ret = NULL;
        g_variant_get(asyncMethodCallResult.get(), "(&s)", &ret);
        if (ret == std::string("resp: arg")) {
            callDone.set();
        }
    };
    conn2->callMethodAsync(TESTAPI_BUS_NAME,
                           TESTAPI_OBJECT_PATH,
                           TESTAPI_INTERFACE,
                           TESTAPI_METHOD_PROCESS,
                           g_variant_new("(s)", "arg"),
                           "(s)",
                           asyncResult2);
    BOOST_REQUIRE(callDone.wait(EVENT_TIMEOUT));

    auto asyncResult3 = [&](dbus::AsyncMethodCallResult& asyncMethodCallResult) {
        try {
            asyncMethodCallResult.get();
        } catch (DbusCustomException&) {
            //expected
            callDone.set();
        }
    };
    conn2->callMethodAsync(TESTAPI_BUS_NAME,
                           TESTAPI_OBJECT_PATH,
                           TESTAPI_INTERFACE,
                           TESTAPI_METHOD_THROW,
                           g_variant_new("(i)", 7),
                           "()",
                           asyncResult3);
    BOOST_REQUIRE(callDone.wait(EVENT_TIMEOUT));
}

BOOST_AUTO_TEST_CASE(MethodAsyncCallAsyncHandler)
{
    ScopedDbusDaemon daemon;
    ScopedGlibLoop loop;
    Latch nameAcquired;
    Latch handlerDone;
    Latch callDone;
    std::string strResult;
    MethodResultBuilder::Pointer deferredResult;

    DbusConnection::Pointer conn1 = DbusConnection::create(DBUS_ADDRESS);
    DbusConnection::Pointer conn2 = DbusConnection::create(DBUS_ADDRESS);

    conn1->setName(TESTAPI_BUS_NAME,
                   [&] {nameAcquired.set();},
                   [] {});
    BOOST_REQUIRE(nameAcquired.wait(EVENT_TIMEOUT));

    auto handler = [&](const std::string& objectPath,
                      const std::string& interface,
                      const std::string& methodName,
                      GVariant* parameters,
                      MethodResultBuilder::Pointer result) {
        if (objectPath != TESTAPI_OBJECT_PATH || interface != TESTAPI_INTERFACE) {
            return;
        }
        if (methodName == TESTAPI_METHOD_PROCESS) {
            const gchar* arg = NULL;
            g_variant_get(parameters, "(&s)", &arg);
            strResult = std::string("resp: ") + arg;
            deferredResult = result;
            handlerDone.set();
        }
    };
    conn1->registerObject(TESTAPI_OBJECT_PATH, TESTAPI_DEFINITION, handler, nullptr);

    auto asyncResult = [&](dbus::AsyncMethodCallResult& asyncMethodCallResult) {
        const gchar* ret = NULL;
        g_variant_get(asyncMethodCallResult.get(), "(&s)", &ret);
        if (ret == std::string("resp: arg")) {
            callDone.set();
        }
    };
    conn2->callMethodAsync(TESTAPI_BUS_NAME,
                           TESTAPI_OBJECT_PATH,
                           TESTAPI_INTERFACE,
                           TESTAPI_METHOD_PROCESS,
                           g_variant_new("(s)", "arg"),
                           "(s)",
                           asyncResult);
    BOOST_REQUIRE(handlerDone.wait(EVENT_TIMEOUT));
    BOOST_REQUIRE(callDone.empty());
    deferredResult->set(g_variant_new("(s)", strResult.c_str()));
    BOOST_REQUIRE(callDone.wait(EVENT_TIMEOUT));
}

BOOST_AUTO_TEST_CASE(MethodCallException)
{
    ScopedDbusDaemon daemon;
    ScopedGlibLoop loop;
    Latch nameAcquired;

    DbusConnection::Pointer conn1 = DbusConnection::create(DBUS_ADDRESS);
    DbusConnection::Pointer conn2 = DbusConnection::create(DBUS_ADDRESS);

    conn1->setName(TESTAPI_BUS_NAME,
                   [&] {nameAcquired.set();},
                   [] {});
    BOOST_REQUIRE(nameAcquired.wait(EVENT_TIMEOUT));
    conn1->registerObject(TESTAPI_OBJECT_PATH,
                          TESTAPI_DEFINITION,
                          DbusConnection::MethodCallCallback(),
                          nullptr);
    BOOST_CHECK_THROW(conn2->callMethod(TESTAPI_BUS_NAME,
                                        TESTAPI_OBJECT_PATH,
                                        TESTAPI_INTERFACE,
                                        TESTAPI_METHOD_NOOP,
                                        NULL,
                                        "()"),
                      DbusOperationException);
    BOOST_CHECK_THROW(conn2->callMethod(TESTAPI_BUS_NAME,
                                        TESTAPI_OBJECT_PATH,
                                        TESTAPI_INTERFACE,
                                        "Foo",
                                        NULL,
                                        "()"),
                      DbusOperationException);
    BOOST_CHECK_THROW(conn2->callMethod(TESTAPI_BUS_NAME,
                                        TESTAPI_OBJECT_PATH,
                                        TESTAPI_INTERFACE + ".foo",
                                        TESTAPI_METHOD_NOOP,
                                        NULL,
                                        "()"),
                      DbusOperationException);
    BOOST_CHECK_THROW(conn2->callMethod(TESTAPI_BUS_NAME,
                                        TESTAPI_OBJECT_PATH + "/foo",
                                        TESTAPI_INTERFACE,
                                        TESTAPI_METHOD_NOOP,
                                        NULL,
                                        "()"),
                      DbusOperationException);
}

BOOST_AUTO_TEST_CASE(DbusApi)
{
    ScopedDbusDaemon daemon;
    ScopedGlibLoop loop;
    DbusTestServer server;
    DbusTestClient client;

    BOOST_CHECK_NO_THROW(client.noop());
    BOOST_CHECK_EQUAL("Processed: arg", client.process("arg"));
    BOOST_CHECK_NO_THROW(client.throwException(0));

    BOOST_CHECK_EXCEPTION(client.throwException(666),
                          DbusCustomException,
                          WhatEquals("Argument: 666"));
}

BOOST_AUTO_TEST_CASE(DbusApiNotify)
{
    ScopedDbusDaemon daemon;
    ScopedGlibLoop loop;
    Latch notified;

    DbusTestServer server;
    DbusTestClient client;

    auto onNotify = [&](const std::string& message) {
        if (message == "notification") {
            notified.set();
        }
    };
    client.setNotifyCallback(onNotify);
    server.notifyClients("notification");
    BOOST_CHECK(notified.wait(EVENT_TIMEOUT));
}

BOOST_AUTO_TEST_CASE(DbusApiNameAcquired)
{
    ScopedDbusDaemon daemon;
    ScopedGlibLoop loop;

    DbusTestServer server;
    DbusTestClient client;

    BOOST_CHECK_THROW(DbusTestServer(), DbusOperationException);
    BOOST_CHECK_NO_THROW(client.noop());
}

BOOST_AUTO_TEST_CASE(DbusApiConnectionLost)
{
    ScopedDbusDaemon daemon;
    ScopedGlibLoop loop;
    Latch disconnected;

    DbusTestServer server;
    server.setDisconnectCallback([&] {disconnected.set();});
    DbusTestClient client;

    BOOST_CHECK_NO_THROW(client.noop());
    daemon.stop();
    BOOST_CHECK(disconnected.wait(EVENT_TIMEOUT));
    BOOST_CHECK_THROW(client.noop(), DbusIOException);
}

BOOST_AUTO_TEST_CASE(DbusApiConnectionLostDelayedCallbackSet)
{
    ScopedDbusDaemon daemon;
    ScopedGlibLoop loop;
    Latch disconnected;

    DbusTestServer server;
    DbusTestClient client;

    BOOST_CHECK_NO_THROW(client.noop());
    daemon.stop();
    BOOST_CHECK_THROW(client.noop(), DbusIOException);

    server.setDisconnectCallback([&] {disconnected.set();});
    BOOST_CHECK(disconnected.wait(EVENT_TIMEOUT));
}

BOOST_AUTO_TEST_SUITE_END()
#endif //DBUS_CONNECTION
