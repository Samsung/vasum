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
 * @brief   Unit tests of the LibvirtNetwork class
 */

#include "config.hpp"
#include "ut.hpp"

#include "libvirt/network.hpp"
#include "libvirt/exception.hpp"

#include <memory>

BOOST_AUTO_TEST_SUITE(LibvirtNetworkSuite)


using namespace security_containers;
using namespace security_containers::libvirt;


namespace {

const std::string CORRECT_CONFIG_XML =  "<network>"
                                        "    <name>test-network</name>"
                                        "    <uuid>44089687-5004-4def-87f0-01c9565f74fd</uuid>"
                                        "    <forward mode='nat'>"
                                        "        <nat>"
                                        "          <port start='1024' end='65535'/>"
                                        "        </nat>"
                                        "    </forward>"
                                        "   <bridge name='test-virbr0' stp='on' delay='0'/>"
                                        "   <ip address='192.168.122.1' netmask='255.255.255.0'>"
                                        "       <dhcp>"
                                        "          <range start='192.168.122.2' end='192.168.122.254'/>"
                                        "       </dhcp>"
                                        "   </ip>"
                                        "</network>";

const std::string BUGGY_CONFIG_XML = "<><TRASH>";

} // namespace

BOOST_AUTO_TEST_CASE(ConstructorDestructorTest)
{
    std::unique_ptr<LibvirtNetwork> netPtr;
    BOOST_REQUIRE_NO_THROW(netPtr.reset(new LibvirtNetwork(CORRECT_CONFIG_XML)));
    BOOST_REQUIRE_NO_THROW(netPtr.reset());
}

BOOST_AUTO_TEST_CASE(BuggyConfigTest)
{
    BOOST_REQUIRE_THROW(LibvirtNetwork net(BUGGY_CONFIG_XML), LibvirtOperationException);
}

BOOST_AUTO_TEST_CASE(DefinitionTest)
{
    LibvirtNetwork net(CORRECT_CONFIG_XML);
    BOOST_CHECK(net.get() != NULL);
}

BOOST_AUTO_TEST_CASE(BoolTest)
{
    LibvirtNetwork net(CORRECT_CONFIG_XML);
    BOOST_CHECK(net);
}

BOOST_AUTO_TEST_SUITE_END()
