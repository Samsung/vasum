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
 * @brief   Unit tests of the LibvirtDomain class
 */

#include "ut.hpp"

#include "libvirt/domain.hpp"
#include "libvirt/exception.hpp"

#include <memory>

BOOST_AUTO_TEST_SUITE(LibvirtDomainSuite)


using namespace security_containers;
using namespace security_containers::libvirt;


namespace {

const std::string CORRECT_CONFIG_XML = "<domain type=\"lxc\">"
                                       "    <name>cnsl</name>"
                                       "    <memory>102400</memory>"
                                       "    <os>"
                                       "        <type>exe</type>"
                                       "        <init>/bin/sh</init>"
                                       "    </os>"
                                       "    <devices>"
                                       "        <console type=\"pty\"/>"
                                       "    </devices>"
                                       "</domain>";
const std::string BUGGY_CONFIG_XML = "<><TRASH>";

} // namespace

BOOST_AUTO_TEST_CASE(ConstructorDestructorTest)
{
    std::unique_ptr<LibvirtDomain> domPtr;
    BOOST_REQUIRE_NO_THROW(domPtr.reset(new LibvirtDomain(CORRECT_CONFIG_XML)));
    BOOST_REQUIRE_NO_THROW(domPtr.reset());
}

BOOST_AUTO_TEST_CASE(BuggyConfigTest)
{
    BOOST_REQUIRE_THROW(LibvirtDomain dom(BUGGY_CONFIG_XML), LibvirtOperationException);
}

BOOST_AUTO_TEST_CASE(DefinitionTest)
{
    LibvirtDomain dom(CORRECT_CONFIG_XML);
    BOOST_CHECK(dom.get() != NULL);
}

BOOST_AUTO_TEST_CASE(BoolTest)
{
    LibvirtDomain dom(CORRECT_CONFIG_XML);
    BOOST_CHECK(dom);
}

BOOST_AUTO_TEST_SUITE_END()
