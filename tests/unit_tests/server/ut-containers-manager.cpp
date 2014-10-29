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
#include "host-dbus-definitions.hpp"
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
using namespace config;
using namespace security_containers::utils;
using namespace dbus;

namespace {

const std::string TEST_CONFIG_PATH = SC_TEST_CONFIG_INSTALL_DIR "/server/ut-containers-manager/test-daemon.conf";
const std::string TEST_DBUS_CONFIG_PATH = SC_TEST_CONFIG_INSTALL_DIR "/server/ut-containers-manager/test-dbus-daemon.conf";
const std::string BUGGY_CONFIG_PATH = SC_TEST_CONFIG_INSTALL_DIR "/server/ut-containers-manager/buggy-daemon.conf";
const std::string BUGGY_FOREGROUND_CONFIG_PATH = SC_TEST_CONFIG_INSTALL_DIR "/server/ut-containers-manager/buggy-foreground-daemon.conf";
const std::string BUGGY_DEFAULTID_CONFIG_PATH = SC_TEST_CONFIG_INSTALL_DIR "/server/ut-containers-manager/buggy-default-daemon.conf";
const std::string TEST_CONTAINER_CONF_PATH = SC_TEST_CONFIG_INSTALL_DIR "/server/ut-containers-manager/containers/";
const std::string MISSING_CONFIG_PATH = "/this/is/a/missing/file/path/missing-daemon.conf";
const int EVENT_TIMEOUT = 5000;
const int TEST_DBUS_CONNECTION_CONTAINERS_COUNT = 3;
const std::string PREFIX_CONSOLE_NAME = "ut-containers-manager-console";
const std::string TEST_APP_NAME = "testapp";
const std::string TEST_MESSAGE = "testmessage";
const std::string FILE_CONTENT = "File content\n"
                                 "Line 1\n"
                                 "Line 2\n";
const std::string NON_EXISTANT_CONTAINER_ID = "NON_EXISTANT_CONTAINER_ID";
const std::string CONTAINERS_PATH = "/tmp/ut-containers"; // the same as in daemon.conf

class DbusAccessory {
public:
    static const int HOST_ID = 0;

    typedef std::function<void(const std::string& argument,
                               MethodResultBuilder::Pointer result
                              )> TestApiMethodCallback;
    typedef std::function<void()> AddContainerResultCallback;

    typedef std::map<std::string, std::string> Dbuses;

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
        mClient->signalSubscribe(callback, isHost() ? api::host::BUS_NAME : api::container::BUS_NAME);
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
        mClient->callMethod(api::container::BUS_NAME,
                            api::container::OBJECT_PATH,
                            api::container::INTERFACE,
                            api::container::METHOD_NOTIFY_ACTIVE_CONTAINER,
                            parameters,
                            "()");
    }

    std::string callMethodMove(const std::string& dest, const std::string& path)
    {
        GVariant* parameters = g_variant_new("(ss)", dest.c_str(), path.c_str());
        GVariantPtr result = mClient->callMethod(api::container::BUS_NAME,
                                                 api::container::OBJECT_PATH,
                                                 api::container::INTERFACE,
                                                 api::container::METHOD_FILE_MOVE_REQUEST,
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
                                                            api::container::BUS_NAME,
                                                 isHost() ? api::host::OBJECT_PATH :
                                                            api::container::OBJECT_PATH,
                                                 isHost() ? api::host::INTERFACE :
                                                            api::container::INTERFACE,
                                                 api::METHOD_PROXY_CALL,
                                                 packedParameters,
                                                 "(v)");
        GVariant* unpackedResult = NULL;
        g_variant_get(result.get(), "(v)", &unpackedResult);
        return GVariantPtr(unpackedResult, g_variant_unref);
    }

    Dbuses callMethodGetContainerDbuses()
    {
        assert(isHost());
        Dbuses dbuses;
        GVariantPtr result = mClient->callMethod(api::host::BUS_NAME,
                                                 api::host::OBJECT_PATH,
                                                 api::host::INTERFACE,
                                                 api::host::METHOD_GET_CONTAINER_DBUSES,
                                                 NULL,
                                                 "(a{ss})");
        GVariant* array = NULL;
        g_variant_get(result.get(), "(*)", &array);
        dbus::GVariantPtr autounref(array, g_variant_unref);
        size_t count = g_variant_n_children(array);
        for (size_t n = 0; n < count; ++n) {
            const char* containerId = NULL;
            const char* dbusAddress = NULL;
            g_variant_get_child(array, n, "{&s&s}", &containerId, &dbusAddress);
            dbuses.insert(Dbuses::value_type(containerId, dbusAddress));
        }
        return dbuses;
    }

    std::vector<std::string> callMethodGetContainerIds()
    {
        assert(isHost());
        GVariantPtr result = mClient->callMethod(api::host::BUS_NAME,
                                                 api::host::OBJECT_PATH,
                                                 api::host::INTERFACE,
                                                 api::host::METHOD_GET_CONTAINER_ID_LIST,
                                                 NULL,
                                                 "(as)");

        GVariant* array = NULL;
        g_variant_get(result.get(), "(*)", &array);

        size_t arraySize = g_variant_n_children(array);
        std::vector<std::string> containerIds;
        for (size_t i = 0; i < arraySize; ++i) {
            const char* id = NULL;
            g_variant_get_child(array, i, "&s", &id);
            containerIds.push_back(id);
        }

        g_variant_unref(array);
        return containerIds;
    }

    std::string callMethodGetActiveContainerId()
    {
        assert(isHost());
        GVariantPtr result = mClient->callMethod(api::host::BUS_NAME,
                                                 api::host::OBJECT_PATH,
                                                 api::host::INTERFACE,
                                                 api::host::METHOD_GET_ACTIVE_CONTAINER_ID,
                                                 NULL,
                                                 "(s)");

        const char* containerId = NULL;
        g_variant_get(result.get(), "(&s)", &containerId);
        return containerId;
    }

    void callMethodSetActiveContainer(const std::string& id)
    {
        assert(isHost());
        GVariant* parameters = g_variant_new("(s)", id.c_str());
        GVariantPtr result = mClient->callMethod(api::host::BUS_NAME,
                                                 api::host::OBJECT_PATH,
                                                 api::host::INTERFACE,
                                                 api::host::METHOD_SET_ACTIVE_CONTAINER,
                                                 parameters,
                                                 "()");

    }

    void callAsyncMethodAddContainer(const std::string& id,
                                     const AddContainerResultCallback& result)
    {
        auto asyncResult = [result](dbus::AsyncMethodCallResult& asyncMethodCallResult) {
            BOOST_CHECK(g_variant_is_of_type(asyncMethodCallResult.get(), G_VARIANT_TYPE_UNIT));
            result();
        };

        assert(isHost());
        GVariant* parameters = g_variant_new("(s)", id.c_str());
        mClient->callMethodAsync(api::host::BUS_NAME,
                                 api::host::OBJECT_PATH,
                                 api::host::INTERFACE,
                                 api::host::METHOD_ADD_CONTAINER,
                                 parameters,
                                 "()",
                                 asyncResult);
    }

private:
    const int mId;
    DbusConnection::Pointer mClient;
    bool mNameAcquired;
    bool mPendingDisconnect;
    std::mutex mMutex;
    std::condition_variable mNameCondition;

    bool isHost() const {
        return mId == HOST_ID;
    }

    std::string acquireAddress() const
    {
        if (isHost()) {
            return "unix:path=/var/run/dbus/system_bus_socket";
        }
        return "unix:path=/tmp/ut-run" + std::to_string(mId) +
               "/dbus/system_bus_socket";
    }
};

std::function<bool(const std::exception&)> expectedMessage(const std::string& message) {
    return [=](const std::exception& e) {
        return e.what() == message;
    };
}

class FileCleanerRAII {
public:
    FileCleanerRAII(const std::vector<std::string>& filePathsToClean):
        mFilePathsToClean(filePathsToClean)
    { }

    ~FileCleanerRAII()
    {
        namespace fs = boost::filesystem;
        for (const auto& file : mFilePathsToClean) {
            fs::path f(file);
            if (fs::exists(f)) {
                fs::remove(f);
            }
        }
    }

private:
    const std::vector<std::string> mFilePathsToClean;
};

struct Fixture {
    security_containers::utils::ScopedGlibLoop mLoop;

    utils::ScopedDir mContainersPathGuard = CONTAINERS_PATH;
    utils::ScopedDir mRun1Guard = utils::ScopedDir("/tmp/ut-run1");
    utils::ScopedDir mRun2Guard = utils::ScopedDir("/tmp/ut-run2");
    utils::ScopedDir mRun3Guard = utils::ScopedDir("/tmp/ut-run3");
};

} // namespace


BOOST_FIXTURE_TEST_SUITE(ContainersManagerSuite, Fixture)

BOOST_AUTO_TEST_CASE(ConstructorDestructorTest)
{
    std::unique_ptr<ContainersManager> cm;
    cm.reset(new ContainersManager(TEST_CONFIG_PATH));
    cm.reset();
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
    cm.startAll();
    BOOST_CHECK(cm.getRunningForegroundContainerId() == "ut-containers-manager-console1");
}

BOOST_AUTO_TEST_CASE(BuggyForegroundTest)
{
    ContainersManager cm(BUGGY_FOREGROUND_CONFIG_PATH);
    cm.startAll();
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
    cm.startAll();
    cm.stopAll();
    BOOST_CHECK(cm.getRunningForegroundContainerId().empty());
}

BOOST_AUTO_TEST_CASE(DetachOnExitTest)
{
    {
        ContainersManager cm(TEST_CONFIG_PATH);
        cm.startAll();
        cm.setContainersDetachOnExit();
    }
    {
        ContainersManager cm(TEST_CONFIG_PATH);
        cm.startAll();
    }
}

BOOST_AUTO_TEST_CASE(FocusTest)
{
    ContainersManager cm(TEST_CONFIG_PATH);
    cm.startAll();
    cm.focus("ut-containers-manager-console2");
    BOOST_CHECK(cm.getRunningForegroundContainerId() == "ut-containers-manager-console2");
    cm.focus("ut-containers-manager-console1");
    BOOST_CHECK(cm.getRunningForegroundContainerId() == "ut-containers-manager-console1");
    cm.focus("ut-containers-manager-console3");
    BOOST_CHECK(cm.getRunningForegroundContainerId() == "ut-containers-manager-console3");
}

BOOST_AUTO_TEST_CASE(NotifyActiveContainerTest)
{
    ContainersManager cm(TEST_DBUS_CONFIG_PATH);
    cm.startAll();

    Latch signalReceivedLatch;
    std::map<int, std::vector<std::string>> signalReceivedSourcesMap;

    std::map<int, std::unique_ptr<DbusAccessory>> dbuses;
    for (int i = 1; i <= TEST_DBUS_CONNECTION_CONTAINERS_COUNT; ++i) {
        dbuses[i] = std::unique_ptr<DbusAccessory>(new DbusAccessory(i));
    }

    auto handler = [](Latch& latch,
                      std::vector<std::string>& receivedSignalSources,
                      const std::string& /*senderBusName*/,
                      const std::string& objectPath,
                      const std::string& interface,
                      const std::string& signalName,
                      GVariant* parameters)
        {
            if (objectPath == api::container::OBJECT_PATH &&
                interface == api::container::INTERFACE &&
                signalName == api::container::SIGNAL_NOTIFICATION &&
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
    cm.startAll();

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
        cm.focus("ut-containers-manager-console3-dbus");

        // emit signal from dbus connection
        client->emitSignal(fake_power_manager_api::OBJECT_PATH,
                           fake_power_manager_api::INTERFACE,
                           fake_power_manager_api::SIGNAL_DISPLAY_OFF,
                           nullptr);

        // check if default container has focus
        BOOST_CHECK(Condition.wait_for(Lock, std::chrono::milliseconds(EVENT_TIMEOUT), cond));
    }
}

BOOST_AUTO_TEST_CASE(MoveFileTest)
{
    ContainersManager cm(TEST_DBUS_CONFIG_PATH);
    cm.startAll();

    Latch notificationLatch;
    std::string notificationSource;
    std::string notificationPath;
    std::string notificationRetcode;

    std::map<int, std::unique_ptr<DbusAccessory>> dbuses;
    for (int i = 1; i <= 2; ++i) {
        dbuses[i] = std::unique_ptr<DbusAccessory>(new DbusAccessory(i));
    }

    auto handler = [&](const std::string& /*senderBusName*/,
                       const std::string& objectPath,
                       const std::string& interface,
                       const std::string& signalName,
                       GVariant* parameters)
        {
            if (objectPath == api::container::OBJECT_PATH &&
                interface == api::container::INTERFACE &&
                signalName == api::container::SIGNAL_NOTIFICATION &&
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

    const std::string TMP = "/tmp/ut-containers";
    const std::string NO_PATH = "path_doesnt_matter_here";
    const std::string BUGGY_PATH = TMP + "/this_file_does_not_exist";
    const std::string BUGGY_CONTAINER = "this-container-does-not-exist";
    const std::string CONTAINER1 = "ut-containers-manager-console1-dbus";
    const std::string CONTAINER2 = "ut-containers-manager-console2-dbus";
    const std::string CONTAINER1PATH = TMP + "/" + CONTAINER1 + TMP;
    const std::string CONTAINER2PATH = TMP + "/" + CONTAINER2 + TMP;

    // sending to a non existing container
    BOOST_CHECK_EQUAL(dbuses.at(1)->callMethodMove(BUGGY_CONTAINER, NO_PATH),
                      api::container::FILE_MOVE_DESTINATION_NOT_FOUND);
    BOOST_CHECK(notificationLatch.empty());

    // sending to self
    BOOST_CHECK_EQUAL(dbuses.at(1)->callMethodMove(CONTAINER1, NO_PATH),
                      api::container::FILE_MOVE_WRONG_DESTINATION);
    BOOST_CHECK(notificationLatch.empty());

    // no permission to send
    BOOST_CHECK_EQUAL(dbuses.at(1)->callMethodMove(CONTAINER2, "/etc/secret1"),
                      api::container::FILE_MOVE_NO_PERMISSIONS_SEND);
    BOOST_CHECK(notificationLatch.empty());

    // no permission to receive
    BOOST_CHECK_EQUAL(dbuses.at(1)->callMethodMove(CONTAINER2, "/etc/secret2"),
                      api::container::FILE_MOVE_NO_PERMISSIONS_RECEIVE);
    BOOST_CHECK(notificationLatch.empty());

    // non existing file
    BOOST_CHECK_EQUAL(dbuses.at(1)->callMethodMove(CONTAINER2, BUGGY_PATH),
                      api::container::FILE_MOVE_FAILED);
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
                      api::container::FILE_MOVE_SUCCEEDED);
    BOOST_CHECK(notificationLatch.wait(EVENT_TIMEOUT));
    BOOST_CHECK(notificationLatch.empty());
    BOOST_CHECK_EQUAL(notificationSource, CONTAINER1);
    BOOST_CHECK_EQUAL(notificationPath, TMP + "/file");
    BOOST_CHECK_EQUAL(notificationRetcode, api::container::FILE_MOVE_SUCCEEDED);
    BOOST_CHECK(!fs::exists(CONTAINER1PATH + "/file"));
    BOOST_CHECK_EQUAL(utils::readFileContent(CONTAINER2PATH + "/file"), FILE_CONTENT);

    fs::remove_all(CONTAINER1PATH, ec);
    fs::remove_all(CONTAINER2PATH, ec);
}

BOOST_AUTO_TEST_CASE(AllowSwitchToDefaultTest)
{
    ContainersManager cm(TEST_DBUS_CONFIG_PATH);
    cm.startAll();

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
        cm.focus("ut-containers-manager-console3-dbus");

        // emit signal from dbus connection
        client->emitSignal(fake_power_manager_api::OBJECT_PATH,
                           fake_power_manager_api::INTERFACE,
                           fake_power_manager_api::SIGNAL_DISPLAY_OFF,
                           nullptr);

        // check if default container has focus
        BOOST_CHECK(condition.wait_for(condLock, std::chrono::milliseconds(EVENT_TIMEOUT), cond));

        // focus non-default container with disabled switching
        cm.focus("ut-containers-manager-console2-dbus");

        // emit signal from dbus connection
        client->emitSignal(fake_power_manager_api::OBJECT_PATH,
                           fake_power_manager_api::INTERFACE,
                           fake_power_manager_api::SIGNAL_DISPLAY_OFF,
                           nullptr);

        // now default container should not be focused
        BOOST_CHECK(!condition.wait_for(condLock, std::chrono::milliseconds(EVENT_TIMEOUT), cond));
    }
}

BOOST_AUTO_TEST_CASE(ProxyCallTest)
{
    ContainersManager cm(TEST_DBUS_CONFIG_PATH);
    cm.startAll();

    std::map<int, std::unique_ptr<DbusAccessory>> dbuses;
    for (int i = 0; i <= TEST_DBUS_CONNECTION_CONTAINERS_COUNT; ++i) {
        dbuses[i] = std::unique_ptr<DbusAccessory>(new DbusAccessory(i));
    }

    for (auto& dbus : dbuses) {
        dbus.second->setName(testapi::BUS_NAME);

        const int id = dbus.first;
        auto handler = [id](const std::string& argument, MethodResultBuilder::Pointer result) {
            if (argument.empty()) {
                result->setError("org.tizen.containers.Error.Test", "Test error");
            } else {
                std::string ret = "reply from " + std::to_string(id) + ": " + argument;
                result->set(g_variant_new("(s)", ret.c_str()));
            }
        };
        dbus.second->registerTestApiObject(handler);
    }

    // host -> container2
    BOOST_CHECK_EQUAL("reply from 2: param1",
                      dbuses.at(0)->testApiProxyCall("ut-containers-manager-console2-dbus",
                                                     "param1"));

    // host -> host
    BOOST_CHECK_EQUAL("reply from 0: param2",
                      dbuses.at(0)->testApiProxyCall("host",
                                                     "param2"));

    // container1 -> host
    BOOST_CHECK_EQUAL("reply from 0: param3",
                      dbuses.at(1)->testApiProxyCall("host",
                                                     "param3"));

    // container1 -> container2
    BOOST_CHECK_EQUAL("reply from 2: param4",
                      dbuses.at(1)->testApiProxyCall("ut-containers-manager-console2-dbus",
                                                     "param4"));

    // container2 -> container2
    BOOST_CHECK_EQUAL("reply from 2: param5",
                      dbuses.at(2)->testApiProxyCall("ut-containers-manager-console2-dbus",
                                                     "param5"));

    // host -> unknown
    BOOST_CHECK_EXCEPTION(dbuses.at(0)->testApiProxyCall("unknown", "param"),
                          DbusCustomException,
                          expectedMessage("Unknown proxy call target"));

    // forwarding error
    BOOST_CHECK_EXCEPTION(dbuses.at(0)->testApiProxyCall("host", ""),
                          DbusCustomException,
                          expectedMessage("Test error"));

    // forbidden call
    BOOST_CHECK_EXCEPTION(dbuses.at(0)->proxyCall("host",
                                              "org.fake",
                                              "/a/b",
                                              "c.d",
                                              "foo",
                                              g_variant_new("(s)", "arg")),
                          DbusCustomException,
                          expectedMessage("Proxy call forbidden"));
}

namespace {
    const DbusAccessory::Dbuses EXPECTED_DBUSES_NO_DBUS = {
        {"ut-containers-manager-console1", ""},
        {"ut-containers-manager-console2", ""},
        {"ut-containers-manager-console3", ""}};

    const DbusAccessory::Dbuses EXPECTED_DBUSES_STOPPED = {
        {"ut-containers-manager-console1-dbus", ""},
        {"ut-containers-manager-console2-dbus", ""},
        {"ut-containers-manager-console3-dbus", ""}};

    const DbusAccessory::Dbuses EXPECTED_DBUSES_STARTED = {
        {"ut-containers-manager-console1-dbus",
         "unix:path=/tmp/ut-run1/dbus/system_bus_socket"},
        {"ut-containers-manager-console2-dbus",
         "unix:path=/tmp/ut-run2/dbus/system_bus_socket"},
        {"ut-containers-manager-console3-dbus",
         "unix:path=/tmp/ut-run3/dbus/system_bus_socket"}};
} // namespace

BOOST_AUTO_TEST_CASE(GetContainerDbusesTest)
{
    DbusAccessory host(DbusAccessory::HOST_ID);
    ContainersManager cm(TEST_DBUS_CONFIG_PATH);

    BOOST_CHECK(EXPECTED_DBUSES_STOPPED == host.callMethodGetContainerDbuses());
    cm.startAll();
    BOOST_CHECK(EXPECTED_DBUSES_STARTED == host.callMethodGetContainerDbuses());
    cm.stopAll();
    BOOST_CHECK(EXPECTED_DBUSES_STOPPED == host.callMethodGetContainerDbuses());
}

BOOST_AUTO_TEST_CASE(GetContainerDbusesNoDbusTest)
{
    DbusAccessory host(DbusAccessory::HOST_ID);
    ContainersManager cm(TEST_CONFIG_PATH);
    BOOST_CHECK(EXPECTED_DBUSES_NO_DBUS == host.callMethodGetContainerDbuses());
    cm.startAll();
    BOOST_CHECK(EXPECTED_DBUSES_NO_DBUS == host.callMethodGetContainerDbuses());
    cm.stopAll();
    BOOST_CHECK(EXPECTED_DBUSES_NO_DBUS == host.callMethodGetContainerDbuses());
}

BOOST_AUTO_TEST_CASE(ContainerDbusesSignalsTest)
{
    Latch signalLatch;
    DbusAccessory::Dbuses collectedDbuses;

    DbusAccessory host(DbusAccessory::HOST_ID);

    auto onSignal = [&] (const std::string& /*senderBusName*/,
                         const std::string& objectPath,
                         const std::string& interface,
                         const std::string& signalName,
                         GVariant* parameters) {
        if (objectPath == api::host::OBJECT_PATH &&
            interface == api::host::INTERFACE &&
            signalName == api::host::SIGNAL_CONTAINER_DBUS_STATE) {

            const gchar* containerId = NULL;
            const gchar* dbusAddress = NULL;
            g_variant_get(parameters, "(&s&s)", &containerId, &dbusAddress);

            collectedDbuses.insert(DbusAccessory::Dbuses::value_type(containerId, dbusAddress));
            signalLatch.set();
        }
    };

    host.signalSubscribe(onSignal);

    {
        ContainersManager cm(TEST_DBUS_CONFIG_PATH);

        BOOST_CHECK(signalLatch.empty());
        BOOST_CHECK(collectedDbuses.empty());

        cm.startAll();

        BOOST_CHECK(signalLatch.waitForN(TEST_DBUS_CONNECTION_CONTAINERS_COUNT, EVENT_TIMEOUT));
        BOOST_CHECK(signalLatch.empty());
        BOOST_CHECK(EXPECTED_DBUSES_STARTED == collectedDbuses);
        collectedDbuses.clear();
    }

    BOOST_CHECK(signalLatch.waitForN(TEST_DBUS_CONNECTION_CONTAINERS_COUNT, EVENT_TIMEOUT));
    BOOST_CHECK(signalLatch.empty());
    BOOST_CHECK(EXPECTED_DBUSES_STOPPED == collectedDbuses);
}


BOOST_AUTO_TEST_CASE(GetContainerIdsTest)
{
    ContainersManager cm(TEST_DBUS_CONFIG_PATH);

    DbusAccessory dbus(DbusAccessory::HOST_ID);

    std::vector<std::string> containerIds = {"ut-containers-manager-console1-dbus",
                                             "ut-containers-manager-console2-dbus",
                                             "ut-containers-manager-console3-dbus"};
    std::vector<std::string> returnedIds = dbus.callMethodGetContainerIds();

    BOOST_CHECK(std::is_permutation(returnedIds.begin(),
                                    returnedIds.end(),
                                    containerIds.begin()));
}

BOOST_AUTO_TEST_CASE(GetActiveContainerIdTest)
{
    ContainersManager cm(TEST_DBUS_CONFIG_PATH);
    cm.startAll();

    DbusAccessory dbus(DbusAccessory::HOST_ID);

    std::vector<std::string> containerIds = {"ut-containers-manager-console1-dbus",
                                             "ut-containers-manager-console2-dbus",
                                             "ut-containers-manager-console3-dbus"};

    for (std::string& containerId: containerIds){
        cm.focus(containerId);
        BOOST_CHECK(dbus.callMethodGetActiveContainerId() == containerId);
    }

    cm.stopAll();
    BOOST_CHECK(dbus.callMethodGetActiveContainerId() == "");
}

BOOST_AUTO_TEST_CASE(SetActiveContainerTest)
{
    ContainersManager cm(TEST_DBUS_CONFIG_PATH);
    cm.startAll();

    DbusAccessory dbus(DbusAccessory::HOST_ID);

    std::vector<std::string> containerIds = {"ut-containers-manager-console1-dbus",
                                             "ut-containers-manager-console2-dbus",
                                             "ut-containers-manager-console3-dbus"};

    for (std::string& containerId: containerIds){
        dbus.callMethodSetActiveContainer(containerId);
        BOOST_CHECK(dbus.callMethodGetActiveContainerId() == containerId);
    }

    BOOST_REQUIRE_THROW(dbus.callMethodSetActiveContainer(NON_EXISTANT_CONTAINER_ID),
                        DbusException);

    cm.stopAll();
    BOOST_REQUIRE_THROW(dbus.callMethodSetActiveContainer("ut-containers-manager-console1-dbus"),
                        DbusException);
}

//TODO fix it
//BOOST_AUTO_TEST_CASE(AddContainerTest)
//{
//    const std::string newContainerId = "test1234";
//    const std::vector<std::string> newContainerConfigs = {
//        TEST_CONTAINER_CONF_PATH + newContainerId + ".conf",
//    };
//    FileCleanerRAII cleaner(newContainerConfigs);
//
//    ContainersManager cm(TEST_DBUS_CONFIG_PATH);
//    cm.startAll();
//
//    Latch callDone;
//    auto resultCallback = [&]() {
//        callDone.set();
//    };
//
//    DbusAccessory dbus(DbusAccessory::HOST_ID);
//
//    // create new container
//    dbus.callAsyncMethodAddContainer(newContainerId, resultCallback);
//    callDone.wait(EVENT_TIMEOUT);
//
//    // focus new container
//    cm.focus(newContainerId);
//    BOOST_CHECK(cm.getRunningForegroundContainerId() == newContainerId);
//}

BOOST_AUTO_TEST_SUITE_END()
