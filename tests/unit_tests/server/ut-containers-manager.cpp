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
 * @brief   Unit tests of the ContainersManager class
 */

#include "config.hpp"
#include "ut.hpp"

#include "containers-manager.hpp"
#include "container-dbus-definitions.hpp"
#include "exception.hpp"

#include "dbus/connection.hpp"
#include "utils/glib-loop.hpp"
#include "config/exception.hpp"
#include "utils/latch.hpp"

#include <memory>
#include <string>
#include <algorithm>
#include <functional>


using namespace security_containers;
using namespace security_containers::config;
using namespace security_containers::utils;
using namespace security_containers::dbus;

namespace {

const std::string TEST_CONFIG_PATH = SC_TEST_CONFIG_INSTALL_DIR "/server/ut-containers-manager/test-daemon.conf";
const std::string TEST_DBUS_CONFIG_PATH = SC_TEST_CONFIG_INSTALL_DIR "/server/ut-containers-manager/test-dbus-daemon.conf";
const std::string BUGGY_CONFIG_PATH = SC_TEST_CONFIG_INSTALL_DIR "/server/ut-containers-manager/buggy-daemon.conf";
const std::string BUGGY_FOREGROUND_CONFIG_PATH = SC_TEST_CONFIG_INSTALL_DIR "/server/ut-containers-manager/buggy-foreground-daemon.conf";
const std::string MISSING_CONFIG_PATH = "/this/is/a/missing/file/path/missing-daemon.conf";
const int EVENT_TIMEOUT = 5000;
const int TEST_DBUS_CONNECTION_CONTAINERS_COUNT = 3;
const std::string PREFIX_CONSOLE_NAME = "ut-containers-manager-console";
const std::string TEST_APP_NAME = "testapp";
const std::string TEST_MESSGAE = "testmessage";

class DbusAccessory {
public:
    std::vector<std::string> mReceivedSignalsSource;

    DbusAccessory(int id, Latch& signalEmittedLatch)
        : mId(id),
          mClient(DbusConnection::create(acquireAddress())),
          mSignalEmittedLatch(signalEmittedLatch)
    {
    }

    void signalSubscribe()
    {
        using namespace std::placeholders;
        mClient->signalSubscribe(std::bind(&DbusAccessory::handler, this, _1, _2, _3, _4, _5),
                                 api::BUS_NAME);
    }

    void callMethod()
    {
        GVariant* parameters = g_variant_new("(ss)", TEST_APP_NAME.c_str(), TEST_MESSGAE.c_str());
        mClient->callMethod(api::BUS_NAME,
                            api::OBJECT_PATH,
                            api::INTERFACE,
                            api::METHOD_NOTIFY_ACTIVE_CONTAINER,
                            parameters,
                            "()");
    }

    std::string getContainerName() const
    {
        return PREFIX_CONSOLE_NAME + std::to_string(mId);
    }

private:
    const int mId;
    DbusConnection::Pointer mClient;
    Latch& mSignalEmittedLatch;

    std::string acquireAddress() const
    {
        return "unix:path=/tmp/ut-containers-manager/console" + std::to_string(mId) +
               "-dbus/dbus/system_bus_socket";
    }

    void handler(const std::string& /*senderBusName*/,
                 const std::string& objectPath,
                 const std::string& interface,
                 const std::string& signalName,
                 GVariant* parameters)
    {
        if (objectPath == api::OBJECT_PATH &&
            interface == api::INTERFACE &&
            signalName == api::SIGNAL_NOTIFICATION &&
            g_variant_is_of_type(parameters, G_VARIANT_TYPE("(sss)"))) {

            const gchar* container = NULL;
            const gchar* application = NULL;
            const gchar* message = NULL;
            g_variant_get(parameters, "(&s&s&s)", &container, &application, &message);
            mReceivedSignalsSource.push_back(container);
            if (application == TEST_APP_NAME && message == TEST_MESSGAE) {
                mSignalEmittedLatch.set();
            }
        }
    }
};


struct Fixture {
    utils::ScopedGlibLoop mLoop;
};

} // namespace


BOOST_FIXTURE_TEST_SUITE(ContainersManagerSuite, Fixture)

BOOST_AUTO_TEST_CASE(ConstructorDestructorTest)
{
    std::unique_ptr<ContainersManager> cm;
    BOOST_REQUIRE_NO_THROW(cm.reset(new ContainersManager(TEST_CONFIG_PATH)));
    BOOST_REQUIRE_NO_THROW(cm.reset());
}

BOOST_AUTO_TEST_CASE(BuggyConfigTest)
{
    BOOST_REQUIRE_THROW(ContainersManager cm(BUGGY_CONFIG_PATH), ConfigException);
}

BOOST_AUTO_TEST_CASE(MissingConfigTest)
{
    BOOST_REQUIRE_THROW(ContainersManager cm(MISSING_CONFIG_PATH), ConfigException);
}

BOOST_AUTO_TEST_CASE(StartAllTest)
{
    ContainersManager cm(TEST_CONFIG_PATH);
    BOOST_REQUIRE_NO_THROW(cm.startAll());
    BOOST_CHECK(cm.getRunningForegroundContainerId() == "ut-containers-manager-console1");
}

BOOST_AUTO_TEST_CASE(BuggyForegroundTest)
{
    ContainersManager cm(BUGGY_FOREGROUND_CONFIG_PATH);
    BOOST_REQUIRE_NO_THROW(cm.startAll());
    BOOST_CHECK(cm.getRunningForegroundContainerId() == "ut-containers-manager-console2");
}

BOOST_AUTO_TEST_CASE(StopAllTest)
{
    ContainersManager cm(TEST_CONFIG_PATH);
    BOOST_REQUIRE_NO_THROW(cm.startAll());
    BOOST_REQUIRE_NO_THROW(cm.stopAll());
    BOOST_CHECK(cm.getRunningForegroundContainerId().empty());
}

BOOST_AUTO_TEST_CASE(DetachOnExitTest)
{
    {
        ContainersManager cm(TEST_CONFIG_PATH);
        BOOST_REQUIRE_NO_THROW(cm.startAll());
        cm.setContainersDetachOnExit();
    }
    {
        ContainersManager cm(TEST_CONFIG_PATH);
        BOOST_REQUIRE_NO_THROW(cm.startAll());
    }
}

BOOST_AUTO_TEST_CASE(FocusTest)
{
    ContainersManager cm(TEST_CONFIG_PATH);
    BOOST_REQUIRE_NO_THROW(cm.startAll());
    BOOST_REQUIRE_NO_THROW(cm.focus("ut-containers-manager-console2"));
    BOOST_CHECK(cm.getRunningForegroundContainerId() == "ut-containers-manager-console2");
    BOOST_REQUIRE_NO_THROW(cm.focus("ut-containers-manager-console1"));
    BOOST_CHECK(cm.getRunningForegroundContainerId() == "ut-containers-manager-console1");
    BOOST_REQUIRE_NO_THROW(cm.focus("ut-containers-manager-console3"));
    BOOST_CHECK(cm.getRunningForegroundContainerId() == "ut-containers-manager-console3");
}

BOOST_AUTO_TEST_CASE(NotifyActiveContainerTest)
{
    Latch signalReceivedLatch;

    ContainersManager cm(TEST_DBUS_CONFIG_PATH);
    cm.startAll();

    std::vector< std::unique_ptr<DbusAccessory> > dbuses;
    for (int i = 1; i <= TEST_DBUS_CONNECTION_CONTAINERS_COUNT; ++i) {
        dbuses.push_back(std::unique_ptr<DbusAccessory>(new DbusAccessory(i, signalReceivedLatch)));
    }
    for (auto& dbus : dbuses) {
        dbus->signalSubscribe();
    }
    for (auto& dbus : dbuses) {
        dbus->callMethod();
    }

    BOOST_CHECK(signalReceivedLatch.waitForN(dbuses.size() - 1, EVENT_TIMEOUT));
    BOOST_CHECK(signalReceivedLatch.empty());

    //check if there are no signals that was received more than once
    for (const auto& source : dbuses[0]->mReceivedSignalsSource) {
        BOOST_CHECK_EQUAL(std::count(dbuses[0]->mReceivedSignalsSource.begin(),
                               dbuses[0]->mReceivedSignalsSource.end(),
                               source), 1);
    }
    //check if all signals was received by active container
    BOOST_CHECK_EQUAL(dbuses[0]->mReceivedSignalsSource.size(), dbuses.size() - 1);
    //check if no signals was received by inactive container
    for (size_t i = 1; i < dbuses.size(); ++i) {
        BOOST_CHECK(dbuses[i]->mReceivedSignalsSource.empty());
    }

    dbuses.clear();
}

BOOST_AUTO_TEST_SUITE_END()
