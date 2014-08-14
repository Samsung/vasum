/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Mateusz Malicki <m.malicki2@samsung.com>
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
 * @author  Mateusz Malicki (m.malicki2@samsung.com)
 * @brief   Unit tests of the client C API
 */

#include <config.hpp>
#include "ut.hpp"
#include <security-containers-client.h>

#include "utils/latch.hpp"
#include "containers-manager.hpp"

#include <map>
#include <string>
#include <utility>
#include <memory>
#include <set>
#include <tuple>
#include <utility>

using namespace security_containers;
using namespace security_containers::utils;

namespace {

const std::string TEST_DBUS_CONFIG_PATH =
    SC_TEST_CONFIG_INSTALL_DIR "/client/ut-client/test-dbus-daemon.conf";

struct Fixture {
    Fixture() { sc_start(); };
    ~Fixture() { sc_stop(); };
};

const int EVENT_TIMEOUT = 5000; ///< ms
const std::map<std::string, std::string> EXPECTED_DBUSES_STARTED = {
    {"ut-containers-manager-console1-dbus",
     "unix:path=/tmp/ut-containers-manager/console1-dbus/dbus/system_bus_socket"},
    {"ut-containers-manager-console2-dbus",
     "unix:path=/tmp/ut-containers-manager/console2-dbus/dbus/system_bus_socket"},
    {"ut-containers-manager-console3-dbus",
     "unix:path=/tmp/ut-containers-manager/console3-dbus/dbus/system_bus_socket"}};

void convertDictToMap(ScArrayString keys,
                      ScArrayString values,
                      std::map<std::string, std::string>& ret)
{
    char** iKeys;
    char** iValues;
    for (iKeys = keys, iValues = values; *iKeys && *iValues; iKeys++, iValues++) {
        ret.insert(std::make_pair(*iKeys, *iValues));
    }
}

void convertArrayToSet(ScArrayString values, std::set<std::string>& ret)
{
    for (char** iValues = values; *iValues; iValues++) {
        ret.insert(*iValues);
    }
}

int getArrayStringLength(ScArrayString astring, int max_len = -1)
{
    int i = 0;
    for (i = 0; astring[i];  i++) {
        if (i == max_len) {
            return max_len;
        }
    }
    return i;
}

Latch signalReceivedLatch;
std::vector< std::tuple<std::string, std::string, std::string> > receivedSignalMsg;

void NotificationTestCallback(const char* container, const char* application, const char* message)
{
    receivedSignalMsg.push_back(std::make_tuple(container, application, message));
    signalReceivedLatch.set();
}

} // namespace

BOOST_FIXTURE_TEST_SUITE(Client, Fixture)

BOOST_AUTO_TEST_CASE(NotRunningServerTest)
{
    ScClient client;
    ScStatus status = sc_get_client(&client,
                                    SCCLIENT_CUSTOM_TYPE,
                                    EXPECTED_DBUSES_STARTED.begin()->second.c_str());
    BOOST_CHECK(sc_is_failed(status));
    sc_client_free(client);
}

BOOST_AUTO_TEST_CASE(GetContainerDbusesTest)
{
    std::unique_ptr<ContainersManager> cm;
    BOOST_REQUIRE_NO_THROW(cm.reset(new ContainersManager(TEST_DBUS_CONFIG_PATH)));
    cm->startAll();
    ScClient client;
    ScStatus status = sc_get_client(&client, SCCLIENT_SYSTEM_TYPE);
    BOOST_REQUIRE(!sc_is_failed(status));
    ScArrayString keys, values;
    status = sc_get_container_dbuses(client, &keys, &values);
    BOOST_REQUIRE(!sc_is_failed(status));

    BOOST_CHECK_EQUAL(getArrayStringLength(keys, EXPECTED_DBUSES_STARTED.size() + 1),
                      EXPECTED_DBUSES_STARTED.size());
    BOOST_CHECK_EQUAL(getArrayStringLength(values, EXPECTED_DBUSES_STARTED.size() + 1),
                      EXPECTED_DBUSES_STARTED.size());

    std::map<std::string, std::string> containers;
    convertDictToMap(keys, values, containers);
    BOOST_CHECK(containers == EXPECTED_DBUSES_STARTED);
    sc_array_string_free(keys);
    sc_array_string_free(values);
    sc_client_free(client);
    BOOST_WARN_NO_THROW(cm.reset());
}

BOOST_AUTO_TEST_CASE(GetContainerIdsTest)
{
    std::unique_ptr<ContainersManager> cm;
    BOOST_REQUIRE_NO_THROW(cm.reset(new ContainersManager(TEST_DBUS_CONFIG_PATH)));
    cm->startAll();
    ScClient client;
    ScStatus status = sc_get_client(&client, SCCLIENT_SYSTEM_TYPE);
    BOOST_REQUIRE(!sc_is_failed(status));
    ScArrayString values;
    status = sc_get_container_ids(client, &values);
    BOOST_REQUIRE(!sc_is_failed(status));
    BOOST_CHECK_EQUAL(getArrayStringLength(values, EXPECTED_DBUSES_STARTED.size() + 1),
                      EXPECTED_DBUSES_STARTED.size());

    std::set<std::string> containers;
    convertArrayToSet(values, containers);

    for (const auto& container : containers) {
        BOOST_CHECK(EXPECTED_DBUSES_STARTED.find(container) != EXPECTED_DBUSES_STARTED.cend());
    }
    sc_array_string_free(values);
    sc_client_free(client);

    BOOST_WARN_NO_THROW(cm.reset());
}

BOOST_AUTO_TEST_CASE(GetActiveContainerIdTest)
{
    std::unique_ptr<ContainersManager> cm;
    BOOST_REQUIRE_NO_THROW(cm.reset(new ContainersManager(TEST_DBUS_CONFIG_PATH)));
    cm->startAll();

    ScClient client;
    ScStatus status = sc_get_client(&client, SCCLIENT_SYSTEM_TYPE);
    BOOST_REQUIRE(!sc_is_failed(status));
    ScString container;
    status = sc_get_active_container_id(client, &container);
    BOOST_REQUIRE(!sc_is_failed(status));

    BOOST_CHECK_EQUAL(container, cm->getRunningForegroundContainerId());

    sc_string_free(container);
    sc_client_free(client);

    BOOST_WARN_NO_THROW(cm.reset());
}

BOOST_AUTO_TEST_CASE(SetActiveContainerTest)
{
    std::string newActiveContainerId = "ut-containers-manager-console2-dbus";

    std::unique_ptr<ContainersManager> cm;
    BOOST_REQUIRE_NO_THROW(cm.reset(new ContainersManager(TEST_DBUS_CONFIG_PATH)));
    cm->startAll();
    BOOST_REQUIRE_NE(newActiveContainerId, cm->getRunningForegroundContainerId());

    ScClient client;
    ScStatus status = sc_get_client(&client, SCCLIENT_SYSTEM_TYPE);
    BOOST_REQUIRE(!sc_is_failed(status));
    status = sc_set_active_container(client, newActiveContainerId.c_str());
    BOOST_REQUIRE(!sc_is_failed(status));
    BOOST_CHECK_EQUAL(newActiveContainerId, cm->getRunningForegroundContainerId());
    sc_client_free(client);
    BOOST_WARN_NO_THROW(cm.reset());
}

BOOST_AUTO_TEST_CASE(NotificationTest)
{
    std::string activeContainerId = "ut-containers-manager-console1-dbus";
    std::unique_ptr<ContainersManager> cm;
    BOOST_REQUIRE_NO_THROW(cm.reset(new ContainersManager(TEST_DBUS_CONFIG_PATH)));
    cm->startAll();

    std::map<std::string, ScClient> clients;
    for (const auto& it : EXPECTED_DBUSES_STARTED) {
        ScClient client;
        ScStatus status = sc_get_client(&client, SCCLIENT_CUSTOM_TYPE, it.second.c_str());
        BOOST_REQUIRE(!sc_is_failed(status));
        clients[it.first] = client;
    }
    for (auto& client : clients) {
        ScStatus status = sc_notification(client.second, NotificationTestCallback);
        BOOST_REQUIRE(!sc_is_failed(status));
    }
    for (auto& client : clients) {
        ScStatus status = sc_notify_active_container(client.second, "app", "msg");
        BOOST_REQUIRE(!sc_is_failed(status));
    }

    BOOST_CHECK(signalReceivedLatch.waitForN(clients.size() - 1, EVENT_TIMEOUT));
    BOOST_CHECK(signalReceivedLatch.empty());

    for (auto& client : clients) {
        sc_client_free(client.second);
    }
    BOOST_WARN_NO_THROW(cm.reset());
}

BOOST_AUTO_TEST_SUITE_END()
