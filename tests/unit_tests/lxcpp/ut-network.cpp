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
#include "logger/logger.hpp"
#include "lxcpp/network-config.hpp"
#include "lxcpp/process.hpp"
#include "utils/execute.hpp"
#include "ut.hpp"

#include <iostream>
#include <sched.h>

#include <net/if.h>

using namespace lxcpp;

namespace {

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

    static void sendCmd(int fd, const char *txt) {
        if (::write(fd, txt, 2) != 2) {
            throw std::runtime_error("pipe write error");
        }
    }
};


int child_exec(void *_fd)
{
    int *fd = (int *)_fd;
    char cmdbuf[2];
    char cmd = '-';

    ::close(fd[1]);
    try {
        lxcpp::NetworkInterface("lo").up();
        for(;;) {
            // child: waiting for parent
            if (::read(fd[0], cmdbuf, 2) != 2) {
                break;
            }

            cmd = cmdbuf[0];

            if (cmd == '0') {
                break;
            } else if (cmd == 'a') {
                const char *argv[] = {
                    "ip", "a",  NULL
                };
                if (!utils::executeAndWait("/sbin/ip", argv)) {
                    throw std::runtime_error("ip addr failed");
                }
            } else if (cmd == 'r') {
                const char *argv[] = {
                    "ip", "route", "list",  NULL
                };
                if (!utils::executeAndWait("/sbin/ip", argv)) {
                    throw std::runtime_error("ip route failed");
                }
            } else if (cmd == 's') {
                const char *argv[] = {
                    "bash", NULL
                };
                if (!utils::executeAndWait("/bin/bash", argv)) {
                    throw std::runtime_error("bash failed");
                }
            } else if (cmd == 'c') {
                LOGW("connecting ... to be done");
            } else {
                continue;
            }
        }

        //cleanup
        ::close(fd[0]);
        _exit(EXIT_SUCCESS);

    } catch(...) {
        _exit(EXIT_FAILURE);
    }
}

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
    BOOST_CHECK_NO_THROW(iflist=NetworkInterface::getInterfaces(0));
    for (const auto& ifn : iflist) {
        const Attrs& attrs = NetworkInterface(ifn).getAttrs();
        BOOST_CHECK(attrs.size() > 0);
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

    cfg.addInetConfig("zone-eth0", InetAddr("1.2.3.4", 24));

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
    std::string name = getUniqueName("test-br");
    NetworkInterface ni(name);
    InetAddr myip("10.100.1.1", 32);

    BOOST_CHECK_NO_THROW(ni.create(InterfaceType::BRIDGE));
    ni.setMACAddress("12:22:33:44:55:66");    // note bit0=0 within first byte !!!
    BOOST_CHECK_NO_THROW(ni.addInetAddr(myip));

    std::vector<std::string> iflist = NetworkInterface::getInterfaces(0);
    BOOST_CHECK(std::find(iflist.begin(), iflist.end(), name) != iflist.end());

    std::vector<InetAddr> addrs = ni.getInetAddressList();
    BOOST_CHECK(std::find(addrs.begin(), addrs.end(), myip) != addrs.end());

    BOOST_CHECK_NO_THROW(ni.delInetAddr(myip));
    BOOST_CHECK_NO_THROW(ni.destroy());
    iflist = NetworkInterface::getInterfaces(0);
    BOOST_CHECK(std::find(iflist.begin(), iflist.end(), ni.getName()) == iflist.end());
}

BOOST_AUTO_TEST_CASE(NetworkMacVLanCreateDestroy)
{
    std::string masterif;
    std::vector<std::string> iflist = NetworkInterface::getInterfaces(0);
    for (const auto& ifn : iflist) {
        if (ifn == "lo") {
            continue;
        }
        NetworkInterface n(ifn);
        if (n.status() == NetStatus::UP) {
            masterif = ifn;
            break;
        }
    }

    NetworkInterface ni(getUniqueName("test-vlan"));
    // creating MACVLAN on masterif
    BOOST_CHECK_NO_THROW(ni.create(InterfaceType::MACVLAN, masterif, MacVLanMode::VEPA));

    iflist = NetworkInterface::getInterfaces(0);
    BOOST_CHECK(std::find(iflist.begin(), iflist.end(), ni.getName()) != iflist.end());

    // destroy MACVLAN
    BOOST_CHECK_NO_THROW(ni.destroy());

    iflist = NetworkInterface::getInterfaces(0);
    BOOST_CHECK(std::find(iflist.begin(), iflist.end(), ni.getName()) == iflist.end());
}

BOOST_AUTO_TEST_CASE(NetworkListRoutes)
{
    unsigned mainLo = 0;
    std::vector<Route> routes;
    // tbl MAIN, all devs
    BOOST_CHECK_NO_THROW(routes = NetworkInterface::getRoutes(0));
    for (auto route : routes) {
        if (route.ifname == "lo") {
            ++mainLo;
        }
    }

    // tbl LOCAL, all devs
    BOOST_CHECK_NO_THROW(routes = NetworkInterface::getRoutes(0,RoutingTable::LOCAL));

    // tbl DEFAULT, all devs
    BOOST_CHECK_NO_THROW(routes = NetworkInterface::getRoutes(0,RoutingTable::DEFAULT));

    NetworkInterface ni("lo");
    // tbl MAIN, dev lo
    BOOST_CHECK_NO_THROW(routes = ni.getRoutes());
    BOOST_CHECK(routes.size() == mainLo);

    // tbl LOCAL, dev lo
    BOOST_CHECK_NO_THROW(routes = ni.getRoutes(RoutingTable::LOCAL));
}

BOOST_AUTO_TEST_CASE(NetworkAddDelRoute)
{
    std::vector<Route> routes;

    Route route = {
        InetAddr("10.100.1.0", 24),//dst - destination network
        InetAddr("", 0),           //src - not specified (prefix=0)
        0,                         // metric
        "",                        // ifname (used only when read routes)
        RoutingTable::UNSPEC       // table (used only when read rotes)
    };

    NetworkInterface ni("lo");

    BOOST_CHECK_NO_THROW(ni.addRoute(route));
    BOOST_CHECK_NO_THROW(routes = ni.getRoutes());
    BOOST_CHECK(std::find_if(routes.begin(), routes.end(),
                            [&route](const Route& item) -> bool {
                                return item.dst == route.dst;
                            }
                         ) != routes.end()
    );

    BOOST_CHECK_NO_THROW(ni.delRoute(route));
    BOOST_CHECK_NO_THROW(routes = ni.getRoutes());
    BOOST_CHECK(std::find_if(routes.begin(), routes.end(),
                            [&route](const Route& item) -> bool {
                                return item.dst == route.dst;
                            }
                         ) == routes.end()
    );
}

BOOST_AUTO_TEST_CASE(NetworkNamespaceCreate)
{
    int fd[2];
    int r = ::pipe(fd);
    BOOST_CHECK(r != -1);

    pid_t pid = lxcpp::clone(child_exec, fd, CLONE_NEWNET);
    ::close(fd[0]);

    //directives for child process
    sendCmd(fd[1], "0"); // exit

    // waiting for child to finish
    int status;
    BOOST_CHECK_NO_THROW(status = lxcpp::waitpid(pid));

    ::close(fd[1]);
    BOOST_CHECK_MESSAGE(status == 0, "child failed");
}

// this test case shows how to create container with network
// Note: this test needs some preparation to successfuly connect an external site:
// 1. allow network forwading (echo 1 > /proc/sys/net/ipv4/ip_forward)
// 2. configure ip masquarading (iptables -t nat -A POSTROUTING -s 10.0.0.0/16 ! -d 10.0.0.0/16 -j MASQUERADE)
BOOST_AUTO_TEST_CASE(NetworkNamespaceVETH)
{
    const char *vbr = "vbr";
    const char *veth1 = "veth-ma";
    const char *veth2 = "veth-sl";

    int fd[2];
    int r = ::pipe(fd);
    BOOST_CHECK(r != -1);

    pid_t pid = lxcpp::clone(child_exec, fd, CLONE_NEWNET);
    ::close(fd[0]);

    NetworkInterface br(vbr);
    NetworkInterface v1(veth1);
    NetworkInterface v2(veth2);

    NetworkInterface("lo", pid).up();

    // creating Bridge vbr
    BOOST_CHECK_NO_THROW(br.create(InterfaceType::BRIDGE));
    BOOST_CHECK_NO_THROW(br.up());
    br.addInetAddr(InetAddr("10.0.0.1", 24));

    // creating VETH pair  veth1 <-> veth2
    BOOST_CHECK_NO_THROW(v1.create(InterfaceType::VETH, v2.getName()));

    // add veth1 to bridge
    BOOST_CHECK_NO_THROW(v1.addToBridge(br.getName()));

    // move veth2 to network namespace (container)
    BOOST_CHECK_NO_THROW(v2.moveToContainer(pid));
    v2.up();
    v2.addInetAddr(InetAddr("10.0.0.2", 24));

    v1.up(); // after v2 up and configured

    // add default route
    v2.addRoute(Route{
        InetAddr("10.0.0.1", 0), //dst - gateway
        InetAddr("", 0),         //src - not specified (prefix=0)
        0,
        "",
        RoutingTable::UNSPEC
    });

    //directives for child process
    sendCmd(fd[1], "a"); // ip addr show
    sendCmd(fd[1], "r"); // ip route list
    sendCmd(fd[1], "c"); // connect extern (needs configured NAT)
    //sendCmd(fd[1], "s"); // exec shell
    sendCmd(fd[1], "0"); // exit

    //  waiting for child to finish
    int status;
    BOOST_CHECK_NO_THROW(status = lxcpp::waitpid(pid));
    ::close(fd[1]);
    BOOST_CHECK_MESSAGE(status == 0, "child failed");

    BOOST_CHECK_NO_THROW(br.destroy());
}

BOOST_AUTO_TEST_SUITE_END()
