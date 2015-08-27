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
#include "lxcpp/network.hpp"
#include "ut.hpp"

#include <iostream>
#include <sched.h>

namespace {

struct Fixture {
    Fixture() {}
    ~Fixture() {}
};

} // namespace

/*
 * NOTE: network inerface unit tests are not finished yet
 * tests are developed/added together with network interface code
 * and container code development
 */

BOOST_FIXTURE_TEST_SUITE(LxcppNetworkSuite, Fixture)

using namespace lxcpp;

BOOST_AUTO_TEST_CASE(ListInterfaces)
{
    std::vector<std::string> iflist;
    BOOST_CHECK_NO_THROW(iflist = NetworkInterface::getInterfaces(0);)
    for (const auto& i : iflist) {
        std::cout << i << std::endl;
    }
}

BOOST_AUTO_TEST_CASE(DetailedListInterfaces)
{
    std::vector<std::string> iflist;
    BOOST_CHECK_NO_THROW(iflist = NetworkInterface::getInterfaces(0);)
    for (const auto& i : iflist) {
        NetworkInterface ni(1,i);
        std::cout << "Attrs of " << i << std::endl;
        Attrs attrs;
        BOOST_CHECK_NO_THROW(attrs = ni.getAttrs();)
        for (const Attr& a : attrs) {
            std::cout << a.name << "=" << a.value << "; ";
        }
        std::cout << std::endl;
    }
}
BOOST_AUTO_TEST_CASE(NetworkConfigSerialization)
{
    NetworkConfig cfg;
    std::string json;
    BOOST_CHECK_NO_THROW(json = config::saveToJsonString(cfg);)
    std::cout << json << std::endl;
}

BOOST_AUTO_TEST_SUITE_END()
