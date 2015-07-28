/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Piotr Bartosiewicz <p.bartosiewi@partner.samsung.com>
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
 * @author  Jan Olszak(j.olszak@samsung.com)
 * @brief   Unit tests of lxcpp Container class
 */

#include "config.hpp"
#include "ut.hpp"

#include "lxcpp/lxcpp.hpp"
#include "lxcpp/exception.hpp"

#include <memory>

namespace {

struct Fixture {
    Fixture() {}
    ~Fixture() {}
};

} // namespace

BOOST_FIXTURE_TEST_SUITE(LxcppContainerSuite, Fixture)

using namespace lxcpp;

BOOST_AUTO_TEST_CASE(ConstructorDestructor)
{
    auto c = createContainer();
    delete c;
}

BOOST_AUTO_TEST_SUITE_END()
