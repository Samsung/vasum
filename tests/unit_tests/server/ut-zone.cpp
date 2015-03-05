/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Lukasz Pawelczyk <l.pawelczyk@partner.samsung.com>
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
 * @author  Lukasz Pawelczyk (l.pawelczyk@partner.samsung.com)
 * @brief   Unit tests of the Zone class
 */

#include "config.hpp"
#include "ut.hpp"

#include "zone.hpp"
#include "netdev.hpp"
#include "exception.hpp"

#include "utils/exception.hpp"
#include "utils/glib-loop.hpp"
#include "utils/scoped-dir.hpp"
#include "config/exception.hpp"
#include "netdev.hpp"

#include <memory>
#include <string>
#include <thread>
#include <chrono>
#include <sys/socket.h>
#include <linux/in6.h>
#include <linux/if_bridge.h>
#include <linux/if.h>

using namespace vasum;
using namespace vasum::netdev;
using namespace config;

namespace {

const std::string TEMPLATES_DIR = VSM_TEST_CONFIG_INSTALL_DIR "/server/ut-zone/templates";
const std::string TEST_CONFIG_PATH = TEMPLATES_DIR + "/test.conf";
const std::string TEST_DBUS_CONFIG_PATH = TEMPLATES_DIR + "/test-dbus.conf";
const std::string BUGGY_CONFIG_PATH = TEMPLATES_DIR + "/buggy.conf";
const std::string MISSING_CONFIG_PATH = TEMPLATES_DIR + "/missing.conf";
const std::string ZONES_PATH = "/tmp/ut-zones";
const std::string LXC_TEMPLATES_PATH = VSM_TEST_LXC_TEMPLATES_INSTALL_DIR;
const std::string DB_PATH = ZONES_PATH + "/vasum.db";
const std::string BRIDGE_NAME = "brtest01";
const std::string ZONE_NETDEV = "netdevtest01";

struct Fixture {
    utils::ScopedGlibLoop mLoop;
    utils::ScopedDir mZonesPathGuard;
    utils::ScopedDir mRunGuard;
    std::string mBridgeName;

    Fixture()
        : mZonesPathGuard(ZONES_PATH)
    {}
    ~Fixture()
    {
        if (!mBridgeName.empty()) {
            try {
                destroyNetdev(mBridgeName);
            } catch (std::exception& ex) {
                BOOST_MESSAGE("Can't destroy bridge: " + std::string(ex.what()));
            }
        }
    }

    std::unique_ptr<Zone> create(const std::string& configPath)
    {
        return std::unique_ptr<Zone>(new Zone(utils::Worker::create(),
                                              "zoneId",
                                              ZONES_PATH,
                                              configPath,
                                              DB_PATH,
                                              LXC_TEMPLATES_PATH,
                                              ""));
    }

    void setupBridge(const std::string& name)
    {
        createBridge(name);
        mBridgeName = name;
    }


    void ensureStarted()
    {
        // wait for zones init to fully start
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    void ensureStop()
    {
        // wait for fully stop
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
};

} // namespace


BOOST_FIXTURE_TEST_SUITE(ZoneSuite, Fixture)

BOOST_AUTO_TEST_CASE(ConstructorDestructorTest)
{
    auto c = create(TEST_CONFIG_PATH);
    c.reset();
}

BOOST_AUTO_TEST_CASE(BuggyConfigTest)
{
    BOOST_REQUIRE_EXCEPTION(create(BUGGY_CONFIG_PATH),
                            ZoneOperationException,
                            WhatEquals("Could not create zone"));
}

BOOST_AUTO_TEST_CASE(MissingConfigTest)
{
    BOOST_REQUIRE_EXCEPTION(create(MISSING_CONFIG_PATH),
                            ConfigException,
                            WhatEquals("Could not load " + MISSING_CONFIG_PATH));
}

BOOST_AUTO_TEST_CASE(StartStopTest)
{
    auto c = create(TEST_CONFIG_PATH);
    c->start();
    ensureStarted();
    c->stop(true);
}

BOOST_AUTO_TEST_CASE(DbusConnectionTest)
{
    mRunGuard.create("/tmp/ut-run"); // the same path as in lxc template

    auto c = create(TEST_DBUS_CONFIG_PATH);
    c->start();
    ensureStarted();
    c->stop(true);
}

// TODO: DbusReconnectionTest

BOOST_AUTO_TEST_CASE(ListNetdevTest)
{
    typedef std::vector<std::string> NetdevList;

    auto c = create(TEST_CONFIG_PATH);
    c->start();
    ensureStarted();
    // Depending on the kernel configuration there can be lots of interfaces (f.e. sit0, ip6tnl0)
    NetdevList netdevs = c->getNetdevList();
    // Check if there is mandatory loopback interface
    BOOST_CHECK(find(netdevs.begin(), netdevs.end(), "lo") != netdevs.end());
    NetdevList hostNetdevs = netdev::listNetdev(0);
    // Check if we get interfaces from zone net namespace
    BOOST_CHECK(hostNetdevs != netdevs);
}

BOOST_AUTO_TEST_CASE(CreateNetdevVethTest)
{
    typedef std::vector<std::string> NetdevList;

    setupBridge(BRIDGE_NAME);
    auto c = create(TEST_CONFIG_PATH);
    c->start();
    ensureStarted();
    c->createNetdevVeth(ZONE_NETDEV, BRIDGE_NAME);
    NetdevList netdevs = c->getNetdevList();
    BOOST_CHECK(find(netdevs.begin(), netdevs.end(), ZONE_NETDEV) != netdevs.end());
    c->stop(false);
    ensureStop();

    //Check clean up
    NetdevList hostNetdevsInit = listNetdev();
    BOOST_REQUIRE_THROW(c->createNetdevVeth(ZONE_NETDEV, BRIDGE_NAME), VasumException);
    NetdevList hostNetdevsThrow = listNetdev();
    BOOST_CHECK_EQUAL_COLLECTIONS(hostNetdevsInit.begin(), hostNetdevsInit.end(),
                                  hostNetdevsThrow.begin(), hostNetdevsThrow.end());
}

BOOST_AUTO_TEST_CASE(CreateNetdevMacvlanTest)
{
    typedef std::vector<std::string> NetdevList;

    setupBridge(BRIDGE_NAME);
    auto c = create(TEST_CONFIG_PATH);
    c->start();
    ensureStarted();
    c->createNetdevVeth(ZONE_NETDEV, BRIDGE_NAME);
    NetdevList netdevs = c->getNetdevList();
    BOOST_CHECK(find(netdevs.begin(), netdevs.end(), ZONE_NETDEV) != netdevs.end());
}

BOOST_AUTO_TEST_CASE(GetNetdevAttrsTest)
{
    setupBridge(BRIDGE_NAME);
    auto c = create(TEST_CONFIG_PATH);
    c->start();
    ensureStarted();
    c->createNetdevVeth(ZONE_NETDEV, BRIDGE_NAME);
    ZoneAdmin::NetdevAttrs attrs = c->getNetdevAttrs(ZONE_NETDEV);
    bool gotMtu = false;
    bool gotFlags = false;
    bool gotType = false;
    for (auto& attr : attrs) {
        if (std::get<0>(attr) == "mtu") {
            BOOST_CHECK(!gotMtu);
            gotMtu = true;
        } else if (std::get<0>(attr) == "flags") {
            BOOST_CHECK(!gotFlags);
            BOOST_CHECK(IFF_BROADCAST & stol(std::get<1>(attr)));
            gotFlags = true;
        } else if (std::get<0>(attr) == "type") {
            BOOST_CHECK(!gotType);
            BOOST_CHECK_EQUAL(1 /*IFF_802_1Q_VLAN */, stol(std::get<1>(attr)));
            gotType = true;
        } else {
            BOOST_CHECK_MESSAGE(false, "Got unexpected option " + std::get<0>(attr));
        }
    }
    BOOST_CHECK(gotMtu);
    BOOST_CHECK(gotFlags);
    BOOST_CHECK(gotType);
}

BOOST_AUTO_TEST_CASE(SetNetdevAttrsTest)
{
    setupBridge(BRIDGE_NAME);
    auto c = create(TEST_CONFIG_PATH);
    c->start();
    ensureStarted();
    c->createNetdevVeth(ZONE_NETDEV, BRIDGE_NAME);
    ZoneAdmin::NetdevAttrs attrsIn;
    attrsIn.push_back(std::make_tuple("mtu", "500"));
    c->setNetdevAttrs(ZONE_NETDEV, attrsIn);

    bool gotMtu = false;
    ZoneAdmin::NetdevAttrs attrsOut = c->getNetdevAttrs(ZONE_NETDEV);
    for (auto& attr : attrsOut) {
        if (std::get<0>(attr) == "mtu") {
            BOOST_CHECK(!gotMtu);
            BOOST_CHECK_EQUAL(std::get<1>(attr), "500");
            gotMtu = true;
        }
    }
    BOOST_CHECK(gotMtu);

    attrsIn.clear();
    attrsIn.push_back(std::make_tuple("does_not_exists", "500"));
    BOOST_REQUIRE_EXCEPTION(c->setNetdevAttrs(ZONE_NETDEV, attrsIn),
                            VasumException,
                            WhatEquals("Unsupported attribute: does_not_exists"));
}

BOOST_AUTO_TEST_CASE(SetNetdevIpv4Test)
{
    setupBridge(BRIDGE_NAME);
    auto c = create(TEST_CONFIG_PATH);
    c->start();
    ensureStarted();
    c->createNetdevVeth(ZONE_NETDEV, BRIDGE_NAME);
    ZoneAdmin::NetdevAttrs attrsIn;
    attrsIn.push_back(std::make_tuple("ipv4", "ip:192.168.4.1,prefixlen:24"));
    c->setNetdevAttrs(ZONE_NETDEV, attrsIn);

    ZoneAdmin::NetdevAttrs attrsOut = c->getNetdevAttrs(ZONE_NETDEV);
    int gotIp = 0;
    for (auto& attr : attrsOut) {
        if (std::get<0>(attr) == "ipv4") {
            BOOST_CHECK(std::get<1>(attr).find("ip:192.168.4.1") != std::string::npos);
            BOOST_CHECK(std::get<1>(attr).find("prefixlen:24") != std::string::npos);
            gotIp++;
        }
    }
    BOOST_CHECK_EQUAL(gotIp, 1);

    attrsIn.clear();
    attrsIn.push_back(std::make_tuple("ipv4", "ip:192.168.4.2,prefixlen:24"));
    attrsIn.push_back(std::make_tuple("ipv4", "ip:192.168.4.3,prefixlen:24"));
    c->setNetdevAttrs(ZONE_NETDEV, attrsIn);
    attrsOut = c->getNetdevAttrs(ZONE_NETDEV);
    gotIp = 0;
    for (auto& attr : attrsOut) {
        if (std::get<0>(attr) == "ipv4") {
            BOOST_CHECK(std::get<1>(attr).find("ip:192.168.4.1") != std::string::npos ||
                        std::get<1>(attr).find("ip:192.168.4.2") != std::string::npos ||
                        std::get<1>(attr).find("ip:192.168.4.3") != std::string::npos);
            BOOST_CHECK(std::get<1>(attr).find("prefixlen:24") != std::string::npos);
            gotIp++;
        }
    }
    BOOST_CHECK_EQUAL(gotIp, 3);
}

BOOST_AUTO_TEST_CASE(SetNetdevIpv6Test)
{
    setupBridge(BRIDGE_NAME);
    auto c = create(TEST_CONFIG_PATH);
    c->start();
    ensureStarted();
    c->createNetdevVeth(ZONE_NETDEV, BRIDGE_NAME);
    ZoneAdmin::NetdevAttrs attrsIn;
    attrsIn.push_back(std::make_tuple("ipv6", "ip:2001:db8::1,prefixlen:64"));
    c->setNetdevAttrs(ZONE_NETDEV, attrsIn);

    ZoneAdmin::NetdevAttrs attrsOut = c->getNetdevAttrs(ZONE_NETDEV);
    int gotIp = 0;
    for (auto& attr : attrsOut) {
        if (std::get<0>(attr) == "ipv6") {
            BOOST_CHECK(std::get<1>(attr).find("ip:2001:db8::1") != std::string::npos);
            BOOST_CHECK(std::get<1>(attr).find("prefixlen:64") != std::string::npos);
            gotIp++;
        }
    }
    BOOST_CHECK_EQUAL(gotIp, 1);

    attrsIn.clear();
    attrsIn.push_back(std::make_tuple("ipv6", "ip:2001:db8::2,prefixlen:64"));
    attrsIn.push_back(std::make_tuple("ipv6", "ip:2001:db8::3,prefixlen:64"));
    c->setNetdevAttrs(ZONE_NETDEV, attrsIn);
    attrsOut = c->getNetdevAttrs(ZONE_NETDEV);
    gotIp = 0;
    for (auto& attr : attrsOut) {
        if (std::get<0>(attr) == "ipv6") {
            BOOST_CHECK(std::get<1>(attr).find("ip:2001:db8::1") != std::string::npos ||
                        std::get<1>(attr).find("ip:2001:db8::2") != std::string::npos ||
                        std::get<1>(attr).find("ip:2001:db8::3") != std::string::npos);
            BOOST_CHECK(std::get<1>(attr).find("prefixlen:64") != std::string::npos);
            gotIp++;
        }
    }
    BOOST_CHECK_EQUAL(gotIp, 3);
}

BOOST_AUTO_TEST_SUITE_END()
