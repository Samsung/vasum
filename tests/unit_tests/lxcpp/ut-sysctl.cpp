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
 * @brief   Unit tests of lxcpp sysctl helpers
 */

#include "config.hpp"
#include "ut.hpp"

#include "lxcpp/sysctl.hpp"
#include "lxcpp/process.hpp"

namespace {

std::string TEST_PARAMETER = "net.ipv4.ip_forward";
std::string WRONG_PARAMETER = "mv.drop_caches";

int clonefn(void* _name)
{
    std::string& name = *static_cast<std::string*>(_name);

    try {
        std::string oldValue = lxcpp::readKernelParameterValue(name);

        std::string newValue = (oldValue == "0" ? "1" : "0");

        lxcpp::writeKernelParameter(name, newValue);

        if (lxcpp::readKernelParameterValue(name) == newValue) {
            ::_exit(EXIT_SUCCESS);
        }
    } catch(...) {}
    ::_exit(EXIT_FAILURE);

    return 0;
}

} // namespace

BOOST_AUTO_TEST_SUITE(LxcppKernelParameterSuite)

using namespace lxcpp;

BOOST_AUTO_TEST_CASE(WriteReadKernelParameter)
{
    pid_t pid = lxcpp::clone(clonefn, static_cast<void*>(&TEST_PARAMETER), CLONE_NEWNET);
    BOOST_REQUIRE(lxcpp::waitpid(pid) == EXIT_SUCCESS);

    pid = lxcpp::clone(clonefn, static_cast<void*>(&WRONG_PARAMETER), CLONE_NEWNET);
    BOOST_REQUIRE(lxcpp::waitpid(pid) == EXIT_FAILURE);
}

BOOST_AUTO_TEST_SUITE_END()
