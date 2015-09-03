/*
 *  Copyright (C) 2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License version 2.1 as published by the Free Software Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

/**
 * @file
 * @author  Krzysztof Dynowski (k.dynowski@samsumg.com)
 * @brief   Unit tests of lxcpp network helpers
 */

#include "config.hpp"
#include "config/manager.hpp"
#include "lxcpp/network-config.hpp"
#include "ut.hpp"

#include <iostream>
#include <sched.h>

#include <net/if.h>

namespace {

using namespace lxcpp;
using namespace config;

struct Fixture {
    Fixture() {}
    ~Fixture() {}

    static std::string getUniqueName(const std::string& prefix) {
        std::vector<std::string> iflist = NetworkInterface::getInterfaces(0);
        std::string name;
        unsigned i = 0;
        do {
            name = prefix + std::to_string(i++);
        } while (std::find(iflist.begin(), iflist.end(), name) != iflist.end());
        return name;
    }
};

} // namespace

/*
 * NOTE: network inerface unit tests are not finished yet
 * tests are developed/added together with network interface code
 * and container code development
 */

BOOST_FIXTURE_TEST_SUITE(LxcppNetworkSuite, Fixture)

BOOST_AUTO_TEST_CASE(NetworkListInterfaces)
{
    std::vector<std::string> iflist;
    BOOST_CHECK_NO_THROW(iflist = NetworkInterface::getInterfaces(0));

    for (const auto& i : iflist) {
        NetworkInterface ni(i);
        std::cout << i << ": ";
        Attrs attrs;
        std::vector<InetAddr> addrs;
        BOOST_CHECK_NO_THROW(attrs = ni.getAttrs());
        for (const Attr& a : attrs) {
            std::cout << a.name << "=" << a.value;
            if (a.name == AttrName::FLAGS) {
                uint32_t f = stoul(a.value);
                std::cout << "(";
                std::cout << ((f & IFF_UP) !=0 ? "UP" : "DONW");
                std::cout << ")";
            }
            std::cout << "; ";
        }
        std::cout << std::endl;

        BOOST_CHECK_NO_THROW(addrs = ni.getInetAddressList());
        for (const InetAddr& a : addrs) {
            std::cout << "   " << toString(a) << std::endl;
        }
    }
}

BOOST_AUTO_TEST_CASE(NetworkConfigSerialization)
{
    std::string tmpConfigFile = "/tmp/netconfig.conf";
    NetworkConfig cfg;
    BOOST_CHECK_NO_THROW(config::saveToJsonString(cfg));

    cfg.addInterfaceConfig("host-veth0", "zone-eth0", InterfaceType::VETH);
    cfg.addInterfaceConfig("host-veth1", "zone-eth1", InterfaceType::BRIDGE);
    cfg.addInterfaceConfig("host-veth2", "zone-eth2", InterfaceType::MACVLAN);

    InetAddr addr(0, 24, "1.2.3.4");
    cfg.addInetConfig("zone-eth0", addr);

    config::saveToJsonFile(tmpConfigFile, cfg);

    NetworkConfig cfg2;
    config::loadFromJsonFile(tmpConfigFile, cfg2);

    int ifnum = cfg.getInterfaces().size();
    for (int i = 0; i < ifnum; ++i) {
        const NetworkInterfaceConfig& ni1 = cfg.getInterface(i);
        const NetworkInterfaceConfig& ni2 = cfg2.getInterface(i);

        BOOST_CHECK_EQUAL(ni1.getHostIf(), ni2.getHostIf());
        BOOST_CHECK_EQUAL(ni1.getZoneIf(), ni2.getZoneIf());
        BOOST_CHECK(ni1.getType() == ni2.getType());
        BOOST_CHECK(ni1.getMode() == ni2.getMode());
    }
}
BOOST_AUTO_TEST_CASE(NetworkBridgeCreateDestroy)
{
    std::string name = getUniqueName("lolo");
    NetworkInterface ni(name);
    InetAddr myip(0, 32, "10.100.1.1");

    BOOST_CHECK_NO_THROW(ni.create(InterfaceType::BRIDGE, ""));
    BOOST_CHECK_NO_THROW(ni.addInetAddr(myip);)

    std::vector<std::string> iflist = NetworkInterface::getInterfaces(0);
    BOOST_CHECK(std::find(iflist.begin(), iflist.end(), name) != iflist.end());

    std::vector<InetAddr> addrs = ni.getInetAddressList();
    BOOST_CHECK(std::find(addrs.begin(), addrs.end(), myip) != addrs.end());
    for (const auto& i : iflist) {
        std::cout << "  " << i;
    }
    std::cout << std::endl;
    for (const InetAddr& a : addrs) {
        std::cout << "   " << toString(a) << std::endl;
    }

    BOOST_CHECK_NO_THROW(ni.delInetAddr(myip));
    BOOST_CHECK_NO_THROW(ni.destroy());
}

BOOST_AUTO_TEST_CASE(NetworkMacVLanCreateDestroy)
{
    std::string masterif;
    std::vector<std::string> iflist = NetworkInterface::getInterfaces(0);
    for (const auto& ifn : iflist) {
        if (ifn == "lo") continue;
        NetworkInterface n(ifn);
        if (n.status() == NetStatus::UP) {
            masterif = ifn;
            break;
        }
    }

    NetworkInterface ni(getUniqueName("lolo"));
    std::cout << " creating MACVLAN on " << masterif << std::endl;
    BOOST_CHECK_NO_THROW(ni.create(InterfaceType::MACVLAN, masterif, MacVLanMode::VEPA));

    iflist = NetworkInterface::getInterfaces(0);
    BOOST_CHECK(std::find(iflist.begin(), iflist.end(), ni.getName()) != iflist.end());

    BOOST_CHECK_NO_THROW(ni.destroy());
}

BOOST_AUTO_TEST_SUITE_END()
