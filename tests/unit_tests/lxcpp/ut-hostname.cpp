/*
 *  Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Dariusz Michaluk (d.michaluk@samsung.com)
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
 * @author  Dariusz Michaluk (d.michaluk@samsung.com)
 * @brief   Unit tests of lxcpp hostname helpers
 */

#include "config.hpp"
#include "ut.hpp"

#include "lxcpp/hostname.hpp"
#include "lxcpp/process.hpp"

namespace {

std::string TEST_NAME =
    "TEST_NAME";
std::string TEST_NAME_MAX =
    "TEST_NAME_TEST_NAME_TEST_NAME_TEST_NAME_TEST_NAME_TEST_NAME_TEST";
std::string TEST_NAME_TO_LONG =
    "TEST_NAME_TEST_NAME_TEST_NAME_TEST_NAME_TEST_NAME_TEST_NAME_TEST_NAME";

int clonefn(void* _hostname)
{
    std::string& hostname = *static_cast<std::string*>(_hostname);

    try {
        lxcpp::setHostName(hostname);

        if (lxcpp::getHostName() == hostname) {
            ::_exit(EXIT_SUCCESS);
        }
    } catch(...) {}
    ::_exit(EXIT_FAILURE);

    return 0;
}

} // namespace

BOOST_AUTO_TEST_SUITE(LxcppHostNameSuite)

using namespace lxcpp;

BOOST_AUTO_TEST_CASE(SetGetHostName)
{
    pid_t pid = lxcpp::clone(clonefn, static_cast<void*>(&TEST_NAME), CLONE_NEWUTS);
    BOOST_REQUIRE(lxcpp::waitpid(pid) == EXIT_SUCCESS);

    pid = lxcpp::clone(clonefn, static_cast<void*>(&TEST_NAME_TO_LONG), CLONE_NEWUTS);
    BOOST_REQUIRE(lxcpp::waitpid(pid) == EXIT_FAILURE);

    pid = lxcpp::clone(clonefn, static_cast<void*>(&TEST_NAME_MAX), CLONE_NEWUTS);
    BOOST_REQUIRE(lxcpp::waitpid(pid) == EXIT_SUCCESS);
}

BOOST_AUTO_TEST_SUITE_END()
