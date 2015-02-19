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
 * @brief   Unit tests of the ZoneConnection class
 */

#include "config.hpp"
#include "ut.hpp"

#include "zone-connection.hpp"
#include "zone-connection-transport.hpp"
#include "host-dbus-definitions.hpp"
#include "zone-dbus-definitions.hpp"
// TODO: Switch to real power-manager dbus defs when they will be implemented in power-manager
#include "fake-power-manager-dbus-definitions.hpp"

#include "dbus/connection.hpp"
#include "dbus/exception.hpp"
#include "utils/scoped-daemon.hpp"
#include "utils/glib-loop.hpp"
#include "utils/latch.hpp"
#include "utils/fs.hpp"
#include "utils/scoped-dir.hpp"


using namespace vasum;
using namespace vasum::utils;
using namespace dbus;

namespace {

const char* DBUS_DAEMON_PROC = "/usr/bin/dbus-daemon";
const char* const DBUS_DAEMON_ARGS[] = {
    DBUS_DAEMON_PROC,
    "--config-file=" VSM_TEST_CONFIG_INSTALL_DIR "/server/ut-zone-connection/ut-dbus.conf",
    "--nofork",
    NULL
};

const std::string ZONES_PATH = "/tmp/ut-zones";
const std::string TRANSPORT_MOUNT_POINT = ZONES_PATH + "/mount-point";
const int EVENT_TIMEOUT = 1000;

class Fixture {
public:
    Fixture()
        : mZonesPathGuard(ZONES_PATH)
        , mTransport(TRANSPORT_MOUNT_POINT)
    {
        mDaemon.start(DBUS_DAEMON_PROC, DBUS_DAEMON_ARGS);
    }

    std::string acquireAddress()
    {
        return mTransport.acquireAddress();
    }
private:
    ScopedGlibLoop mLoop;
    ScopedDir mZonesPathGuard;
    ZoneConnectionTransport mTransport;
    ScopedDaemon mDaemon;
};

class DbusNameSetter {
public:
    DbusNameSetter()
        : mNameAcquired(false),
          mPendingDisconnect(false)
    {
    }

    void setName(const std::unique_ptr<DbusConnection>& conn, const std::string& name)
    {
        conn->setName(name,
                      std::bind(&DbusNameSetter::onNameAcquired, this),
                      std::bind(&DbusNameSetter::onDisconnect, this));

        if(!waitForName()) {
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

private:
    bool mNameAcquired;
    bool mPendingDisconnect;
    std::mutex mMutex;
    std::condition_variable mNameCondition;
};

} // namespace

BOOST_FIXTURE_TEST_SUITE(ZoneConnectionSuite, Fixture)

BOOST_AUTO_TEST_CASE(ConstructorDestructorConnectTest)
{
    ZoneConnection(acquireAddress(), nullptr);
}

BOOST_AUTO_TEST_CASE(NotifyActiveZoneApiTest)
{
    Latch notifyCalled;
    ZoneConnection connection(acquireAddress(), nullptr);

    auto callback = [&](const std::string& application, const std::string& message) {
        if (application == "testapp" && message == "testmessage") {
            notifyCalled.set();
        }
    };
    connection.setNotifyActiveZoneCallback(callback);

    DbusConnection::Pointer client = DbusConnection::create(acquireAddress());
    client->callMethod(api::zone::BUS_NAME,
                       api::zone::OBJECT_PATH,
                       api::zone::INTERFACE,
                       api::zone::METHOD_NOTIFY_ACTIVE_ZONE,
                       g_variant_new("(ss)", "testapp", "testmessage"),
                       "()");
    BOOST_CHECK(notifyCalled.wait(EVENT_TIMEOUT));
}

BOOST_AUTO_TEST_CASE(SignalNotificationApiTest)
{
    Latch signalEmitted;
    ZoneConnection connection(acquireAddress(), nullptr);

    DbusConnection::Pointer client = DbusConnection::create(acquireAddress());

    auto handler = [&](const std::string& /*senderBusName*/,
                       const std::string& objectPath,
                       const std::string& interface,
                       const std::string& signalName,
                       GVariant* parameters) {
        if (objectPath == api::zone::OBJECT_PATH &&
            interface == api::zone::INTERFACE &&
            signalName == api::zone::SIGNAL_NOTIFICATION &&
            g_variant_is_of_type(parameters, G_VARIANT_TYPE("(sss)"))) {

            const gchar* zone = NULL;
            const gchar* application = NULL;
            const gchar* message = NULL;
            g_variant_get(parameters, "(&s&s&s)", &zone, &application, &message);
            if (zone == std::string("testzone") &&
                application == std::string("testapp") &&
                message == std::string("testmessage")) {
                signalEmitted.set();
            }
        }
    };
    client->signalSubscribe(handler, api::zone::BUS_NAME);

    connection.sendNotification("testzone", "testapp", "testmessage");

    BOOST_CHECK(signalEmitted.wait(EVENT_TIMEOUT));
}

BOOST_AUTO_TEST_CASE(SignalDisplayOffApiTest)
{
    Latch displayOffCalled;
    ZoneConnection connection(acquireAddress(), nullptr);

    DbusConnection::Pointer client = DbusConnection::create(acquireAddress());

    auto callback = [&]() {
        displayOffCalled.set();
    };

    connection.setDisplayOffCallback(callback);

    client->emitSignal(fake_power_manager_api::OBJECT_PATH,
                       fake_power_manager_api::INTERFACE,
                       fake_power_manager_api::SIGNAL_DISPLAY_OFF,
                       nullptr);

    // timeout should occur, since no name is set to client
    BOOST_CHECK(!displayOffCalled.wait(EVENT_TIMEOUT));

    DbusNameSetter setter;

    setter.setName(client, fake_power_manager_api::BUS_NAME);

    client->emitSignal(fake_power_manager_api::OBJECT_PATH,
                       fake_power_manager_api::INTERFACE,
                       fake_power_manager_api::SIGNAL_DISPLAY_OFF,
                       nullptr);

    // now signal should be delivered correctly
    BOOST_CHECK(displayOffCalled.wait(EVENT_TIMEOUT));
}


BOOST_AUTO_TEST_SUITE_END()
