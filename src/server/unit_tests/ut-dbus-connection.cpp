/*
 *  Copyright (c) 2000 - 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Bumjin Im <bj.im@samsung.com>
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
 * @file    ut-dbus-connection.cpp
 * @author  Piotr Bartosiewicz (p.bartosiewi@partner.samsung.com)
 * @brief   Dbus connection unit tests
 */

#include "ut.hpp"
#include "dbus-connection.hpp"
#include "dbus-exception.hpp"
#include "utils-scoped-daemon.hpp"
#include "utils-glib-loop.hpp"
#include "utils-file-wait.hpp"
#include "utils-latch.hpp"
#include "dbus-test-server.hpp"
#include "dbus-test-client.hpp"
#include "dbus-test-common.hpp"
#include "log.hpp"
#include <thread>
#include <mutex>
#include <condition_variable>


BOOST_AUTO_TEST_SUITE(DbusSuite)

using namespace security_containers;
using namespace security_containers::utils;
using namespace security_containers::dbus;

namespace {

const char* DBUS_DAEMON_PROC = "/bin/dbus-daemon";
const char* const DBUS_DAEMON_ARGS[] = {
    DBUS_DAEMON_PROC,
    "--config-file=/etc/security-containers/config/tests/ut-dbus-connection/ut-dbus.conf",
    "--nofork",
    NULL
};
const int DBUS_DAEMON_TIMEOUT = 1000;
const int EVENT_TIMEOUT = 1000;

class ScopedDbusDaemon : public ScopedDaemon {
public:
    ScopedDbusDaemon() : ScopedDaemon(DBUS_DAEMON_PROC, DBUS_DAEMON_ARGS)
    {
        waitForFile(DBUS_SOCKET_FILE, DBUS_DAEMON_TIMEOUT);
    }
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

    DbusConnection::Pointer conn1 = DbusConnection::create(DBUS_ADDRESS);
    DbusConnection::Pointer conn2 = DbusConnection::create(DBUS_ADDRESS);

    // acquire name by conn1
    Latch nameAcquired1, nameLost1;
    conn1->setName(TESTAPI_BUS_NAME,
                   [&] {nameAcquired1.set();},
                   [&] {nameLost1.set();});
    BOOST_CHECK(nameAcquired1.wait(EVENT_TIMEOUT));
    BOOST_CHECK(nameLost1.empty());

    // conn2 can't acquire name
    Latch nameAcquired2, nameLost2;
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

BOOST_AUTO_TEST_CASE(SignalTest)
{
    ScopedDbusDaemon daemon;
    ScopedGlibLoop loop;
    DbusConnection::Pointer conn1 = DbusConnection::create(DBUS_ADDRESS);
    DbusConnection::Pointer conn2 = DbusConnection::create(DBUS_ADDRESS);
    conn2->signalSubscribe();
    conn1->emitSignal("/a/b/c", "a.b.c", "Foo", NULL);
    sleep(1);
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
    DbusConnection::Pointer conn1 = DbusConnection::create(DBUS_ADDRESS);
    DbusConnection::Pointer conn2 = DbusConnection::create(DBUS_ADDRESS);

    Latch nameAcquired;
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
}

BOOST_AUTO_TEST_CASE(MethodCallTest)
{
    ScopedDbusDaemon daemon;
    ScopedGlibLoop loop;
    DbusConnection::Pointer conn1 = DbusConnection::create(DBUS_ADDRESS);
    DbusConnection::Pointer conn2 = DbusConnection::create(DBUS_ADDRESS);

    Latch nameAcquired;
    conn1->setName(TESTAPI_BUS_NAME,
                   [&] {nameAcquired.set();},
                   [] {});
    BOOST_REQUIRE(nameAcquired.wait(EVENT_TIMEOUT));
    auto handler = [] (const std::string&, const std::string&, const std::string& method,
                       GVariant* /*parameters*/, MethodResultBuilder& result) {
        if (method == TESTAPI_METHOD_NOOP) {
            result.setVoid();
        }
    };
    conn1->registerObject(TESTAPI_OBJECT_PATH, TESTAPI_DEFINITION, handler);
    GVariantPtr result = conn2->callMethod(TESTAPI_BUS_NAME,
                                           TESTAPI_OBJECT_PATH,
                                           TESTAPI_INTERFACE,
                                           TESTAPI_METHOD_NOOP,
                                           NULL,
                                           "()");
    BOOST_CHECK(g_variant_is_of_type(result.get(), G_VARIANT_TYPE_UNIT));
}

BOOST_AUTO_TEST_CASE(MethodCallExceptionTest)
{
    ScopedDbusDaemon daemon;
    ScopedGlibLoop loop;
    DbusConnection::Pointer conn1 = DbusConnection::create(DBUS_ADDRESS);
    DbusConnection::Pointer conn2 = DbusConnection::create(DBUS_ADDRESS);

    Latch nameAcquired;
    conn1->setName(TESTAPI_BUS_NAME,
                   [&] {nameAcquired.set();},
                   [] {});
    BOOST_REQUIRE(nameAcquired.wait(EVENT_TIMEOUT));
    conn1->registerObject(TESTAPI_OBJECT_PATH, TESTAPI_DEFINITION,
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
    DbusTestServer server;
    DbusTestClient client;

    BOOST_CHECK_NO_THROW(client.noop());
    daemon.stop();
    BOOST_CHECK_THROW(client.noop(), DbusIOException);

    Latch disconnected;
    server.setDisconnectCallback([&] {disconnected.set();});
    BOOST_CHECK(disconnected.wait(EVENT_TIMEOUT));
}

BOOST_AUTO_TEST_SUITE_END()
