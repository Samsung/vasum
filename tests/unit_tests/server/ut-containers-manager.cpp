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
// TODO: Switch to real power-manager dbus defs when they will be implemented in power-manager
#include "fake-power-manager-dbus-definitions.hpp"
#include "exception.hpp"

#include "dbus/connection.hpp"
#include "dbus/exception.hpp"
#include "utils/glib-loop.hpp"
#include "config/exception.hpp"
#include "utils/latch.hpp"
#include "utils/fs.hpp"

#include <vector>
#include <map>
#include <memory>
#include <string>
#include <algorithm>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <boost/filesystem.hpp>

using namespace security_containers;
using namespace security_containers::config;
using namespace security_containers::utils;
using namespace security_containers::dbus;

namespace {

const std::string TEST_CONFIG_PATH = SC_TEST_CONFIG_INSTALL_DIR "/server/ut-containers-manager/test-daemon.conf";
const std::string TEST_DBUS_CONFIG_PATH = SC_TEST_CONFIG_INSTALL_DIR "/server/ut-containers-manager/test-dbus-daemon.conf";
const std::string BUGGY_CONFIG_PATH = SC_TEST_CONFIG_INSTALL_DIR "/server/ut-containers-manager/buggy-daemon.conf";
const std::string BUGGY_FOREGROUND_CONFIG_PATH = SC_TEST_CONFIG_INSTALL_DIR "/server/ut-containers-manager/buggy-foreground-daemon.conf";
const std::string BUGGY_DEFAULTID_CONFIG_PATH = SC_TEST_CONFIG_INSTALL_DIR "/server/ut-containers-manager/buggy-default-daemon.conf";
const std::string MISSING_CONFIG_PATH = "/this/is/a/missing/file/path/missing-daemon.conf";
const int EVENT_TIMEOUT = 5000;
const int TEST_DBUS_CONNECTION_CONTAINERS_COUNT = 3;
const std::string PREFIX_CONSOLE_NAME = "ut-containers-manager-console";
const std::string TEST_APP_NAME = "testapp";
const std::string TEST_MESSAGE = "testmessage";
const std::string FILE_CONTENT = "File content\n"
                                 "Line 1\n"
                                 "Line 2\n";

class DbusAccessory {
public:
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
        mClient->signalSubscribe(callback, api::BUS_NAME);
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
        mClient->callMethod(api::BUS_NAME,
                            api::OBJECT_PATH,
                            api::INTERFACE,
                            api::METHOD_NOTIFY_ACTIVE_CONTAINER,
                            parameters,
                            "()");
    }

    std::string callMethodMove(const std::string& dest, const std::string& path)
    {
        GVariant* parameters = g_variant_new("(ss)", dest.c_str(), path.c_str());
        GVariantPtr result = mClient->callMethod(api::BUS_NAME,
                                                 api::OBJECT_PATH,
                                                 api::INTERFACE,
                                                 api::METHOD_FILE_MOVE_REQUEST,
                                                 parameters,
                                                 "(s)");

        const gchar* retcode = NULL;
        g_variant_get(result.get(), "(&s)", &retcode);
        return std::string(retcode);
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
        return "unix:path=/tmp/ut-containers-manager/console" + std::to_string(mId) +
               "-dbus/dbus/system_bus_socket";
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

BOOST_AUTO_TEST_CASE(BuggyDefaultTest)
{
    BOOST_REQUIRE_THROW(ContainersManager cm(BUGGY_DEFAULTID_CONFIG_PATH),
                        ContainerOperationException);
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
    ContainersManager cm(TEST_DBUS_CONFIG_PATH);
    cm.startAll();

    std::map<int, std::unique_ptr<DbusAccessory>> dbuses;
    for (int i = 1; i <= TEST_DBUS_CONNECTION_CONTAINERS_COUNT; ++i) {
        dbuses[i] = std::unique_ptr<DbusAccessory>(new DbusAccessory(i));
    }

    Latch signalReceivedLatch;
    std::map<int, std::vector<std::string>> signalReceivedSourcesMap;
    auto handler = [](Latch& latch,
                      std::vector<std::string>& receivedSignalSources,
                      const std::string& /*senderBusName*/,
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
                receivedSignalSources.push_back(container);
                if (application == TEST_APP_NAME && message == TEST_MESSAGE) {
                    latch.set();
                }
            }
        };

    using namespace std::placeholders;
    for (int i = 1; i <= TEST_DBUS_CONNECTION_CONTAINERS_COUNT; ++i) {
        dbuses[i]->signalSubscribe(std::bind(handler,
                                             std::ref(signalReceivedLatch),
                                             std::ref(signalReceivedSourcesMap[i]),
                                             _1, _2, _3, _4, _5));
    }
    for (auto& dbus : dbuses) {
        dbus.second->callMethodNotify();
    }

    BOOST_CHECK(signalReceivedLatch.waitForN(dbuses.size() - 1, EVENT_TIMEOUT));
    BOOST_CHECK(signalReceivedLatch.empty());

    //check if there are no signals that was received more than once
    for (const auto& source : signalReceivedSourcesMap[1]) {
        BOOST_CHECK_EQUAL(std::count(signalReceivedSourcesMap[1].begin(),
                                     signalReceivedSourcesMap[1].end(),
                                     source), 1);
    }
    //check if all signals was received by active container
    BOOST_CHECK_EQUAL(signalReceivedSourcesMap[1].size(), dbuses.size() - 1);
    //check if no signals was received by inactive container
    for (size_t i = 2; i <= dbuses.size(); ++i) {
        BOOST_CHECK(signalReceivedSourcesMap[i].empty());
    }

    dbuses.clear();
}

BOOST_AUTO_TEST_CASE(DisplayOffTest)
{
    ContainersManager cm(TEST_DBUS_CONFIG_PATH);
    BOOST_REQUIRE_NO_THROW(cm.startAll());

    std::vector<std::unique_ptr<DbusAccessory>> clients;
    for (int i = 1; i <= TEST_DBUS_CONNECTION_CONTAINERS_COUNT; ++i) {
        clients.push_back(std::unique_ptr<DbusAccessory>(new DbusAccessory(i)));
    }

    for (auto& client : clients) {
        client->setName(fake_power_manager_api::BUS_NAME);
    }

    std::mutex Mutex;
    std::unique_lock<std::mutex> Lock(Mutex);
    std::condition_variable Condition;
    auto cond = [&cm]() -> bool {
        return cm.getRunningForegroundContainerId() == "ut-containers-manager-console1-dbus";
    };

    for (auto& client : clients) {
        // TEST SWITCHING TO DEFAULT CONTAINER
        // focus non-default container
        BOOST_REQUIRE_NO_THROW(cm.focus("ut-containers-manager-console3-dbus"));

        // emit signal from dbus connection
        BOOST_REQUIRE_NO_THROW(client->emitSignal(fake_power_manager_api::OBJECT_PATH,
                                                  fake_power_manager_api::INTERFACE,
                                                  fake_power_manager_api::SIGNAL_DISPLAY_OFF,
                                                  nullptr));

        // check if default container has focus
        BOOST_CHECK(Condition.wait_for(Lock, std::chrono::milliseconds(EVENT_TIMEOUT), cond));
    }
}

BOOST_AUTO_TEST_CASE(MoveFileTest)
{
    ContainersManager cm(TEST_DBUS_CONFIG_PATH);
    cm.startAll();

    std::map<int, std::unique_ptr<DbusAccessory>> dbuses;
    for (int i = 1; i <= 2; ++i) {
        dbuses[i] = std::unique_ptr<DbusAccessory>(new DbusAccessory(i));
    }

    Latch notificationLatch;
    std::string notificationSource;
    std::string notificationPath;
    std::string notificationRetcode;
    auto handler = [&](const std::string& /*senderBusName*/,
                       const std::string& objectPath,
                       const std::string& interface,
                       const std::string& signalName,
                       GVariant* parameters)
        {
            if (objectPath == api::OBJECT_PATH &&
                interface == api::INTERFACE &&
                signalName == api::SIGNAL_NOTIFICATION &&
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

    // subscribe the second (destination) container for notifications
    dbuses.at(2)->signalSubscribe(handler);

    const std::string TMP = "/tmp";
    const std::string NO_PATH = "path_doesnt_matter_here";
    const std::string BUGGY_PATH = TMP + "/this_file_does_not_exist";
    const std::string BUGGY_CONTAINER = "this-container-does-not-exist";
    const std::string CONTAINER1 = "ut-containers-manager-console1-dbus";
    const std::string CONTAINER2 = "ut-containers-manager-console2-dbus";
    const std::string CONTAINER1PATH = TMP + "/" + CONTAINER1 + TMP;
    const std::string CONTAINER2PATH = TMP + "/" + CONTAINER2 + TMP;

    // sending to a non existing container
    BOOST_CHECK_EQUAL(dbuses.at(1)->callMethodMove(BUGGY_CONTAINER, NO_PATH),
                      api::FILE_MOVE_DESTINATION_NOT_FOUND);
    BOOST_CHECK(notificationLatch.empty());

    // sending to self
    BOOST_CHECK_EQUAL(dbuses.at(1)->callMethodMove(CONTAINER1, NO_PATH),
                      api::FILE_MOVE_WRONG_DESTINATION);
    BOOST_CHECK(notificationLatch.empty());

    // no permission to send
    BOOST_CHECK_EQUAL(dbuses.at(1)->callMethodMove(CONTAINER2, "/etc/secret1"),
                      api::FILE_MOVE_NO_PERMISSIONS_SEND);
    BOOST_CHECK(notificationLatch.empty());

    // no permission to receive
    BOOST_CHECK_EQUAL(dbuses.at(1)->callMethodMove(CONTAINER2, "/etc/secret2"),
                      api::FILE_MOVE_NO_PERMISSIONS_RECEIVE);
    BOOST_CHECK(notificationLatch.empty());

    // non existing file
    BOOST_CHECK_EQUAL(dbuses.at(1)->callMethodMove(CONTAINER2, BUGGY_PATH),
                      api::FILE_MOVE_FAILED);
    BOOST_CHECK(notificationLatch.empty());

    // a working scenario
    namespace fs = boost::filesystem;
    boost::system::error_code ec;
    fs::remove_all(CONTAINER1PATH, ec);
    fs::remove_all(CONTAINER2PATH, ec);
    BOOST_REQUIRE(fs::create_directories(CONTAINER1PATH, ec));
    BOOST_REQUIRE(fs::create_directories(CONTAINER2PATH, ec));
    BOOST_REQUIRE(utils::saveFileContent(CONTAINER1PATH + "/file", FILE_CONTENT));

    BOOST_CHECK_EQUAL(dbuses.at(1)->callMethodMove(CONTAINER2, TMP + "/file"),
                      api::FILE_MOVE_SUCCEEDED);
    BOOST_CHECK(notificationLatch.wait(EVENT_TIMEOUT));
    BOOST_CHECK(notificationLatch.empty());
    BOOST_CHECK_EQUAL(notificationSource, CONTAINER1);
    BOOST_CHECK_EQUAL(notificationPath, TMP + "/file");
    BOOST_CHECK_EQUAL(notificationRetcode, api::FILE_MOVE_SUCCEEDED);
    BOOST_CHECK(!fs::exists(CONTAINER1PATH + "/file"));
    BOOST_CHECK_EQUAL(utils::readFileContent(CONTAINER2PATH + "/file"), FILE_CONTENT);

    fs::remove_all(CONTAINER1PATH, ec);
    fs::remove_all(CONTAINER2PATH, ec);
}

BOOST_AUTO_TEST_CASE(AllowSwitchToDefaultTest)
{
    ContainersManager cm(TEST_DBUS_CONFIG_PATH);
    BOOST_REQUIRE_NO_THROW(cm.startAll());

    std::vector<std::unique_ptr<DbusAccessory>> clients;
    for (int i = 1; i <= TEST_DBUS_CONNECTION_CONTAINERS_COUNT; ++i) {
        clients.push_back(std::unique_ptr<DbusAccessory>(new DbusAccessory(i)));
    }

    for (auto& client : clients) {
        client->setName(fake_power_manager_api::BUS_NAME);
    }

    std::mutex condMutex;
    std::unique_lock<std::mutex> condLock(condMutex);
    std::condition_variable condition;
    auto cond = [&cm]() -> bool {
        return cm.getRunningForegroundContainerId() == "ut-containers-manager-console1-dbus";
    };

    for (auto& client : clients) {
        // focus non-default container with allowed switching
        BOOST_REQUIRE_NO_THROW(cm.focus("ut-containers-manager-console3-dbus"));

        // emit signal from dbus connection
        BOOST_REQUIRE_NO_THROW(client->emitSignal(fake_power_manager_api::OBJECT_PATH,
                                                  fake_power_manager_api::INTERFACE,
                                                  fake_power_manager_api::SIGNAL_DISPLAY_OFF,
                                                  nullptr));

        // check if default container has focus
        BOOST_CHECK(condition.wait_for(condLock, std::chrono::milliseconds(EVENT_TIMEOUT), cond));

        // focus non-default container with disabled switching
        BOOST_REQUIRE_NO_THROW(cm.focus("ut-containers-manager-console2-dbus"));

        // emit signal from dbus connection
        BOOST_REQUIRE_NO_THROW(client->emitSignal(fake_power_manager_api::OBJECT_PATH,
                                                  fake_power_manager_api::INTERFACE,
                                                  fake_power_manager_api::SIGNAL_DISPLAY_OFF,
                                                  nullptr));

        // now default container should not be focused
        BOOST_CHECK(!condition.wait_for(condLock, std::chrono::milliseconds(EVENT_TIMEOUT), cond));
    }
}


BOOST_AUTO_TEST_SUITE_END()
