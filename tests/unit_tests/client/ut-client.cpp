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
#include "container-dbus-definitions.hpp"

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

struct Loop {
    Loop()
    {
        vsm_start_glib_loop();
    }
    ~Loop()
    {
        vsm_stop_glib_loop();
    }
};

struct Fixture {
    Loop loop;
    ContainersManager cm;

    Fixture(): cm(TEST_DBUS_CONFIG_PATH)
    {
        cm.startAll();
    }
};

const int EVENT_TIMEOUT = 5000; ///< ms
const std::map<std::string, std::string> EXPECTED_DBUSES_STARTED = {
    {
        "ut-containers-manager-console1-dbus",
        "unix:path=/tmp/ut-containers-manager/console1-dbus/dbus/system_bus_socket"
    },
    {
        "ut-containers-manager-console2-dbus",
        "unix:path=/tmp/ut-containers-manager/console2-dbus/dbus/system_bus_socket"
    },
    {
        "ut-containers-manager-console3-dbus",
        "unix:path=/tmp/ut-containers-manager/console3-dbus/dbus/system_bus_socket"
    }
};

void convertDictToMap(VsmArrayString keys,
                      VsmArrayString values,
                      std::map<std::string, std::string>& ret)
{
    VsmArrayString iKeys;
    VsmArrayString iValues;
    for (iKeys = keys, iValues = values; *iKeys && *iValues; iKeys++, iValues++) {
        ret.insert(std::make_pair(*iKeys, *iValues));
    }
}

void convertArrayToSet(VsmArrayString values, std::set<std::string>& ret)
{
    for (VsmArrayString iValues = values; *iValues; iValues++) {
        ret.insert(*iValues);
    }
}

int getArrayStringLength(VsmArrayString astring, int max_len = -1)
{
    int i = 0;
    for (i = 0; astring[i];  i++) {
        if (i == max_len) {
            return max_len;
        }
    }
    return i;
}

} // namespace

BOOST_FIXTURE_TEST_SUITE(Client, Fixture)

BOOST_AUTO_TEST_CASE(NotRunningServerTest)
{
    cm.stopAll();

    VsmClient client = vsm_client_create();
    VsmStatus status = vsm_connect_custom(client,
                                          EXPECTED_DBUSES_STARTED.begin()->second.c_str());
    BOOST_CHECK_EQUAL(VSMCLIENT_IO_ERROR, status);
    vsm_client_free(client);
}

BOOST_AUTO_TEST_CASE(GetContainerDbusesTest)
{
    VsmClient client = vsm_client_create();
    VsmStatus status = vsm_connect(client);
    BOOST_REQUIRE_EQUAL(VSMCLIENT_SUCCESS, status);
    VsmArrayString keys, values;
    status = vsm_get_container_dbuses(client, &keys, &values);
    BOOST_REQUIRE_EQUAL(VSMCLIENT_SUCCESS, status);

    BOOST_CHECK_EQUAL(getArrayStringLength(keys, EXPECTED_DBUSES_STARTED.size() + 1),
                      EXPECTED_DBUSES_STARTED.size());
    BOOST_CHECK_EQUAL(getArrayStringLength(values, EXPECTED_DBUSES_STARTED.size() + 1),
                      EXPECTED_DBUSES_STARTED.size());

    std::map<std::string, std::string> containers;
    convertDictToMap(keys, values, containers);
    BOOST_CHECK(containers == EXPECTED_DBUSES_STARTED);
    vsm_array_string_free(keys);
    vsm_array_string_free(values);
    vsm_client_free(client);
}

BOOST_AUTO_TEST_CASE(GetContainerIdsTest)
{
    VsmClient client = vsm_client_create();
    VsmStatus status = vsm_connect(client);
    BOOST_REQUIRE_EQUAL(VSMCLIENT_SUCCESS, status);
    VsmArrayString values;
    status = vsm_get_domain_ids(client, &values);
    BOOST_REQUIRE_EQUAL(VSMCLIENT_SUCCESS, status);
    BOOST_CHECK_EQUAL(getArrayStringLength(values, EXPECTED_DBUSES_STARTED.size() + 1),
                      EXPECTED_DBUSES_STARTED.size());

    std::set<std::string> containers;
    convertArrayToSet(values, containers);

    for (const auto& container : containers) {
        BOOST_CHECK(EXPECTED_DBUSES_STARTED.find(container) != EXPECTED_DBUSES_STARTED.cend());
    }
    vsm_array_string_free(values);
    vsm_client_free(client);
}

BOOST_AUTO_TEST_CASE(GetActiveContainerIdTest)
{
    VsmClient client = vsm_client_create();
    VsmStatus status = vsm_connect(client);
    BOOST_REQUIRE_EQUAL(VSMCLIENT_SUCCESS, status);
    VsmString container;
    status = vsm_get_active_container_id(client, &container);
    BOOST_REQUIRE_EQUAL(VSMCLIENT_SUCCESS, status);

    BOOST_CHECK_EQUAL(container, cm.getRunningForegroundContainerId());

    vsm_string_free(container);
    vsm_client_free(client);
}

BOOST_AUTO_TEST_CASE(SetActiveContainerTest)
{
    const std::string newActiveContainerId = "ut-containers-manager-console2-dbus";

    BOOST_REQUIRE_NE(newActiveContainerId, cm.getRunningForegroundContainerId());

    VsmClient client = vsm_client_create();
    VsmStatus status = vsm_connect(client);
    BOOST_REQUIRE_EQUAL(VSMCLIENT_SUCCESS, status);
    status = vsm_set_active_container(client, newActiveContainerId.c_str());
    BOOST_REQUIRE_EQUAL(VSMCLIENT_SUCCESS, status);
    BOOST_CHECK_EQUAL(newActiveContainerId, cm.getRunningForegroundContainerId());
    vsm_client_free(client);
}

BOOST_AUTO_TEST_CASE(AddContainerTest)
{
    const std::string newActiveContainerId = "";

    VsmClient client = vsm_client_create();
    VsmStatus status = vsm_connect(client);
    BOOST_REQUIRE_EQUAL(VSMCLIENT_SUCCESS, status);
    status = vsm_create_domain(client, newActiveContainerId.c_str());
    BOOST_REQUIRE_EQUAL(VSMCLIENT_CUSTOM_ERROR, status);
    vsm_client_free(client);
}

BOOST_AUTO_TEST_CASE(FileMoveRequestTest)
{
    const std::string path = "/tmp/fake_path";
    const std::string secondContainer = "fake_container";

    VsmClient client = vsm_client_create();
    VsmStatus status = vsm_connect_custom(client, EXPECTED_DBUSES_STARTED.begin()->second.c_str());
    BOOST_REQUIRE_EQUAL(VSMCLIENT_SUCCESS, status);
    status = vsm_file_move_request(client, secondContainer.c_str(), path.c_str());
    BOOST_REQUIRE_EQUAL(VSMCLIENT_CUSTOM_ERROR, status);
    BOOST_REQUIRE_EQUAL(api::container::FILE_MOVE_DESTINATION_NOT_FOUND,
                        vsm_get_status_message(client));
    vsm_client_free(client);
}

BOOST_AUTO_TEST_CASE(NotificationTest)
{
    const std::string MSG_CONTENT = "msg";
    const std::string MSG_APP = "app";

    struct CallbackData {
        Latch signalReceivedLatch;
        std::vector< std::tuple<std::string, std::string, std::string> > receivedSignalMsg;
    };

    auto callback = [](const char* container,
                       const char* application,
                       const char* message,
    void* data) {
        CallbackData& callbackData = *reinterpret_cast<CallbackData*>(data);
        callbackData.receivedSignalMsg.push_back(std::make_tuple(container, application, message));
        callbackData.signalReceivedLatch.set();
    };

    CallbackData callbackData;
    std::map<std::string, VsmClient> clients;
    for (const auto& it : EXPECTED_DBUSES_STARTED) {
        VsmClient client = vsm_client_create();
        VsmStatus status = vsm_connect_custom(client, it.second.c_str());
        BOOST_REQUIRE_EQUAL(VSMCLIENT_SUCCESS, status);
        clients[it.first] = client;
    }
    for (auto& client : clients) {
        VsmStatus status = vsm_notification(client.second, callback, &callbackData);
        BOOST_REQUIRE_EQUAL(VSMCLIENT_SUCCESS, status);
    }
    for (auto& client : clients) {
        VsmStatus status = vsm_notify_active_container(client.second,
                                                       MSG_APP.c_str(),
                                                       MSG_CONTENT.c_str());
        BOOST_REQUIRE_EQUAL(VSMCLIENT_SUCCESS, status);
    }

    BOOST_CHECK(callbackData.signalReceivedLatch.waitForN(clients.size() - 1, EVENT_TIMEOUT));
    BOOST_CHECK(callbackData.signalReceivedLatch.empty());

    for (const auto& msg : callbackData.receivedSignalMsg) {
        BOOST_CHECK(clients.count(std::get<0>(msg)) > 0);
        BOOST_CHECK_EQUAL(std::get<1>(msg), MSG_APP);
        BOOST_CHECK_EQUAL(std::get<2>(msg), MSG_CONTENT);
    }

    for (auto& client : clients) {
        vsm_client_free(client.second);
    }
}

BOOST_AUTO_TEST_CASE(GetContainerIdByPidTest1)
{
    VsmClient client = vsm_client_create();
    VsmString container;
    VsmStatus status = vsm_lookup_domain_by_pid(client, 1, &container);
    BOOST_REQUIRE_EQUAL(VSMCLIENT_SUCCESS, status);

    BOOST_CHECK_EQUAL(container, std::string("host"));

    vsm_string_free(container);
    vsm_client_free(client);
}

BOOST_AUTO_TEST_CASE(GetContainerIdByPidTest2)
{
    std::set<std::string> ids;

    VsmClient client = vsm_client_create();
    for (int n = 0; n < 100000; ++n) {
        VsmString container;
        VsmStatus status = vsm_lookup_domain_by_pid(client, n, &container);
        if (status == VSMCLIENT_SUCCESS) {
            ids.insert(container);
            vsm_string_free(container);
        } else {
            BOOST_WARN_MESSAGE(status == VSMCLIENT_INVALID_ARGUMENT, vsm_get_status_message(client));
        }
    }
    vsm_client_free(client);

    BOOST_CHECK(ids.count("host") == 1);

    for (const auto& dbus : EXPECTED_DBUSES_STARTED) {
        BOOST_CHECK(ids.count(dbus.first) == 1);
    }
}

BOOST_AUTO_TEST_SUITE_END()
