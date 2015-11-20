/*
 *  Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Jan Olszak (j.olszak@samsung.com)
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
 * @brief   Unit tests of lxcpp namespace helpers
 */

#include "config.hpp"
#include "ut.hpp"

#include "lxcpp/namespace.hpp"
#include <iostream>
#include <sched.h>

namespace {

struct Fixture {
    Fixture() {}
    ~Fixture() {}
};

} // namespace

BOOST_FIXTURE_TEST_SUITE(LxcppNamespaceSuite, Fixture)

using namespace lxcpp;

static const std::array<int, 6> NAMESPACES {{
        CLONE_NEWUSER,
        CLONE_NEWNS,
        CLONE_NEWPID,
        CLONE_NEWUTS,
        CLONE_NEWIPC,
        CLONE_NEWNET
    }};

BOOST_AUTO_TEST_CASE(GetPath)
{
    for(const auto ns: NAMESPACES) {
        getPath(0, ns);
    }
}

BOOST_AUTO_TEST_CASE(ToString)
{
    for(const auto ns: NAMESPACES) {
        nsToString(ns);
    }
}

BOOST_AUTO_TEST_SUITE_END()
