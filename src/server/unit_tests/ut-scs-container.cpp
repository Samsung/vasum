/*
 *  Copyright (c) 2000 - 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Bumjin Im <bj.im@samsung.com>
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
 * @file    ut-scs-container.cpp
 * @author  Jan Olszak (j.olszak@samsung.com)
 * @brief   Unit tests of the Container class
 */

#include "scs-container.hpp"

#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

using namespace security_containers;

BOOST_AUTO_TEST_SUITE(ContainerSuite)

BOOST_AUTO_TEST_CASE(SimpleTest)
{
    Container c;
    c.define();
    c.start();
    c.stop();
    c.undefine();
}

BOOST_AUTO_TEST_SUITE_END()
