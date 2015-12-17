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

#include "lxcpp/environment.hpp"
#include "lxcpp/exception.hpp"
#include "lxcpp/namespace.hpp"
#include "lxcpp/process.hpp"

#include <iostream>
#include <sched.h>
#include <stdlib.h>

namespace {

const int TEST_PASSED = 0;
const int ERROR = 1;

const std::string TEST_NAME = "TEST_NAME";
const std::string TEST_VALUE = "TEST_VALUE";

const std::string TEST_NAME_REMOVED = "TEST_NAME_REMOVED";
const std::string TEST_VALUE_REMOVED = "TEST_VALUE_REMOVED";

} // namespace

BOOST_AUTO_TEST_SUITE(LxcppEnvironmentSuite)

using namespace lxcpp;

BOOST_AUTO_TEST_CASE(SetGetEnv)
{
    pid_t pid = lxcpp::fork();
    if (pid == 0) {
        try {
            lxcpp::setenv(TEST_NAME, TEST_VALUE);

            if(lxcpp::getenv(TEST_NAME) == TEST_VALUE) {
                ::_exit(TEST_PASSED);
            }
            ::_exit(ERROR);
        } catch(...) {
            ::_exit(ERROR);
        }
    }

    BOOST_REQUIRE(lxcpp::waitpid(pid) == TEST_PASSED);
}

BOOST_AUTO_TEST_CASE(ClearEnvExcept)
{
    pid_t pid = lxcpp::fork();
    if (pid == 0) {
        try {
            lxcpp::setenv(TEST_NAME, TEST_VALUE);
            lxcpp::setenv(TEST_NAME_REMOVED, TEST_VALUE_REMOVED);

            clearenvExcept({TEST_NAME});

            try {
                lxcpp::getenv(TEST_NAME_REMOVED);
            } catch(const lxcpp::NoSuchValue&) {
                if(lxcpp::getenv(TEST_NAME) == TEST_VALUE) {
                    ::_exit(TEST_PASSED);
                }
            }
            ::_exit(ERROR);
        } catch(...) {
            ::_exit(ERROR);
        }
    }

    BOOST_REQUIRE(lxcpp::waitpid(pid) == TEST_PASSED);
}

BOOST_AUTO_TEST_CASE(ClearEnv)
{
    pid_t pid = lxcpp::fork();
    if (pid == 0) {
        try {
            lxcpp::setenv(TEST_NAME_REMOVED, TEST_VALUE_REMOVED);
            lxcpp::clearenv();

            // clearenv should clear environ
            if (::environ) {
                ::_exit(ERROR);
            }

            try {
                lxcpp::getenv(TEST_NAME_REMOVED);
            } catch(const lxcpp::NoSuchValue&) {
                ::_exit(TEST_PASSED);
            }
            ::_exit(ERROR);
        } catch(...) {
            ::_exit(ERROR);
        }
    }

    BOOST_REQUIRE(lxcpp::waitpid(pid) == TEST_PASSED);
}

BOOST_AUTO_TEST_SUITE_END()
