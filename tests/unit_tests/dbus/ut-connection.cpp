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

#include "config.hpp"
#include "ut.hpp"
#include "dbus/test-server.hpp"
#include "dbus/test-client.hpp"
#include "dbus/test-common.hpp"
#include "utils/scoped-daemon.hpp"

#include "dbus/connection.hpp"
#include "dbus/exception.hpp"
#include "utils/glib-loop.hpp"
#include "utils/file-wait.hpp"
#include "utils/latch.hpp"
#include "utils/fs.hpp"
#include "log/logger.hpp"

#include <boost/filesystem.hpp>
#include <thread>
#include <mutex>
#include <condition_variable>


BOOST_AUTO_TEST_SUITE(DbusSuite)

using namespace security_containers;
using namespace security_containers::utils;
using namespace security_containers::dbus;

namespace {

const char* DBUS_DAEMON_PROC = "/usr/bin/dbus-daemon";
const char* const DBUS_DAEMON_ARGS[] = {
    DBUS_DAEMON_PROC,
    "--config-file=" SC_TEST_CONFIG_INSTALL_DIR "/dbus/ut-connection/ut-dbus.conf",
    "--nofork",
    NULL
};
const int DBUS_DAEMON_TIMEOUT = 1000;
const int EVENT_TIMEOUT = 1000;

class ScopedDbusDaemon {
public:
    ScopedDbusDaemon()
    {
        boost::filesystem::remove("/tmp/container_socket");
        mDaemon.start(DBUS_DAEMON_PROC, DBUS_DAEMON_ARGS);
        waitForFile(DBUS_SOCKET_FILE, DBUS_DAEMON_TIMEOUT);
    }
    void stop()
    {
        mDaemon.stop();
    }
private:
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

BOOST_AUTO_TEST_CASE(GlibLoopTest)
{
    ScopedGlibLoop loop;
}

BOOST_AUTO_TEST_CASE(DbusDaemonTest)
{
    ScopedDbusDaemon daemon;
}

BOOST_AUTO_TEST_CASE(NoDbusTest)
{
    ScopedGlibLoop loop;
    BOOST_CHECK_THROW(DbusConnection::create(DBUS_ADDRESS), DbusIOException);
}

BOOST_AUTO_TEST_CASE(SimpleTest)
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

BOOST_AUTO_TEST_CASE(ConnectionLostTest)
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

BOOST_AUTO_TEST_CASE(NameOwnerTest)
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

BOOST_AUTO_TEST_CASE(GenericSignalTest)
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

BOOST_AUTO_TEST_CASE(FilteredSignalTest)
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

BOOST_AUTO_TEST_CASE(RegisterObjectTest)
{
    ScopedDbusDaemon daemon;
    ScopedGlibLoop loop;
    DbusConnection::Pointer conn = DbusConnection::create(DBUS_ADDRESS);
    DbusConnection::MethodCallCallback callback;
    BOOST_CHECK_THROW(conn->registerObject(TESTAPI_OBJECT_PATH, "<invalid", callback),
                      DbusInvalidArgumentException);
    BOOST_CHECK_THROW(conn->registerObject(TESTAPI_OBJECT_PATH, "", callback),
                      DbusInvalidArgumentException);
    BOOST_CHECK_THROW(conn->registerObject(TESTAPI_OBJECT_PATH, "<node></node>", callback),
                      DbusInvalidArgumentException);
    BOOST_CHECK_NO_THROW(conn->registerObject(TESTAPI_OBJECT_PATH, TESTAPI_DEFINITION, callback));
}

BOOST_AUTO_TEST_CASE(IntrospectSystemTest)
{
    ScopedDbusDaemon daemon;
    ScopedGlibLoop loop;
    DbusConnection::Pointer conn = DbusConnection::createSystem();
    std::string xml = conn->introspect("org.freedesktop.DBus", "/org/freedesktop/DBus");
    std::string iface = getInterfaceFromIntrospectionXML(xml, "org.freedesktop.DBus");
    BOOST_CHECK(!iface.empty());
}

BOOST_AUTO_TEST_CASE(IntrospectTest)
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
                          DbusConnection::MethodCallCallback());
    std::string xml = conn2->introspect(TESTAPI_BUS_NAME, TESTAPI_OBJECT_PATH);
    std::string iface = getInterfaceFromIntrospectionXML(xml, TESTAPI_INTERFACE);
    BOOST_REQUIRE(!iface.empty());
    BOOST_CHECK(std::string::npos != iface.find(TESTAPI_INTERFACE));
    BOOST_CHECK(std::string::npos != iface.find(TESTAPI_METHOD_NOOP));
    BOOST_CHECK(std::string::npos != iface.find(TESTAPI_METHOD_PROCESS));
    BOOST_CHECK(std::string::npos != iface.find(TESTAPI_METHOD_THROW));
    BOOST_CHECK(std::string::npos != iface.find(TESTAPI_SIGNAL_NOTIFY));
}

BOOST_AUTO_TEST_CASE(MethodCallTest)
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
                      MethodResultBuilder& result) {
        if (objectPath != TESTAPI_OBJECT_PATH || interface != TESTAPI_INTERFACE) {
            return;
        }
        if (methodName == TESTAPI_METHOD_NOOP) {
            result.setVoid();
        } else if (methodName == TESTAPI_METHOD_PROCESS) {
            const gchar* arg = NULL;
            g_variant_get(parameters, "(&s)", &arg);
            std::string str = std::string("resp: ") + arg;
            result.set(g_variant_new("(s)", str.c_str()));
        } else if (methodName == TESTAPI_METHOD_THROW) {
            int arg = 0;
            g_variant_get(parameters, "(i)", &arg);
            result.setError("org.tizen.containers.Error.Test", "msg: " + std::to_string(arg));
        }
    };
    conn1->registerObject(TESTAPI_OBJECT_PATH, TESTAPI_DEFINITION, handler);

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

BOOST_AUTO_TEST_CASE(MethodAsyncCallTest)
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
                      MethodResultBuilder& result) {
        if (objectPath != TESTAPI_OBJECT_PATH || interface != TESTAPI_INTERFACE) {
            return;
        }
        if (methodName == TESTAPI_METHOD_NOOP) {
            result.setVoid();
        } else if (methodName == TESTAPI_METHOD_PROCESS) {
            const gchar* arg = NULL;
            g_variant_get(parameters, "(&s)", &arg);
            std::string str = std::string("resp: ") + arg;
            result.set(g_variant_new("(s)", str.c_str()));
        } else if (methodName == TESTAPI_METHOD_THROW) {
            int arg = 0;
            g_variant_get(parameters, "(i)", &arg);
            result.setError("org.tizen.containers.Error.Test", "msg: " + std::to_string(arg));
        }
    };
    conn1->registerObject(TESTAPI_OBJECT_PATH, TESTAPI_DEFINITION, handler);

    Latch callDone;

    auto asyncResult1 = [&](dbus::AsyncMethodCallResult& asyncMethodCallResult) {
        BOOST_CHECK(g_variant_is_of_type(asyncMethodCallResult.get(), G_VARIANT_TYPE_UNIT));
        callDone.set();
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
        BOOST_CHECK_EQUAL("resp: arg", ret);
        callDone.set();
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
        BOOST_CHECK_THROW(asyncMethodCallResult.get(), DbusCustomException);
        callDone.set();
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

BOOST_AUTO_TEST_CASE(MethodCallExceptionTest)
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
                          DbusConnection::MethodCallCallback());
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

BOOST_AUTO_TEST_CASE(DbusApiTest)
{
    ScopedDbusDaemon daemon;
    ScopedGlibLoop loop;
    DbusTestServer server;
    DbusTestClient client;

    BOOST_CHECK_NO_THROW(client.noop());
    BOOST_CHECK_EQUAL("Processed: arg", client.process("arg"));
    BOOST_CHECK_NO_THROW(client.throwException(0));

    auto checkException = [](const DbusCustomException& e) {
        return e.what() == std::string("Argument: 666");
    };
    BOOST_CHECK_EXCEPTION(client.throwException(666), DbusCustomException, checkException);
}

BOOST_AUTO_TEST_CASE(DbusApiNotifyTest)
{
    ScopedDbusDaemon daemon;
    ScopedGlibLoop loop;
    Latch notified;

    DbusTestServer server;
    DbusTestClient client;

    auto onNotify = [&](const std::string& message) {
        BOOST_CHECK_EQUAL("notification", message);
        notified.set();
    };
    client.setNotifyCallback(onNotify);
    server.notifyClients("notification");
    BOOST_CHECK(notified.wait(EVENT_TIMEOUT));
}

BOOST_AUTO_TEST_CASE(DbusApiNameAcquiredTest)
{
    ScopedDbusDaemon daemon;
    ScopedGlibLoop loop;

    DbusTestServer server;
    DbusTestClient client;

    BOOST_CHECK_THROW(DbusTestServer(), DbusOperationException);
    BOOST_CHECK_NO_THROW(client.noop());
}

BOOST_AUTO_TEST_CASE(DbusApiConnectionLost1Test)
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

BOOST_AUTO_TEST_CASE(DbusApiConnectionLost2Test)
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
