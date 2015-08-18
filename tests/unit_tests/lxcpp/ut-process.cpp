/*
 *  Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Jan Olszak(j.olszak@samsung.com)
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
 * @brief   Unit tests of lxcpp process helpers
 */

#include "config.hpp"
#include "ut.hpp"

#include "lxcpp/process.hpp"
#include "lxcpp/exception.hpp"
#include "utils/exception.hpp"
#include "utils/execute.hpp"

#include <iostream>
#include <sched.h>
#include <sys/wait.h>

namespace {

struct Fixture {
    Fixture() {}
    ~Fixture() {}
};

int clonefn(void* /*args*/) {
    return 0;
}

} // namespace

BOOST_FIXTURE_TEST_SUITE(LxcppProcessSuite, Fixture)

using namespace lxcpp;

const std::vector<Namespace> NAMESPACES  {{
        Namespace::USER,
        Namespace::MNT,
        Namespace::PID,
        Namespace::UTS,
        Namespace::IPC,
        Namespace::NET
    }};

BOOST_AUTO_TEST_CASE(Clone)
{
    BOOST_CHECK_NO_THROW(lxcpp::clone(clonefn, nullptr, NAMESPACES));
    BOOST_CHECK_NO_THROW(lxcpp::clone(clonefn, nullptr, {Namespace::MNT}));
}

BOOST_AUTO_TEST_CASE(Setns)
{
    const int TEST_PASSED = 0;
    const int ERROR = 1;

    pid_t pid = lxcpp::fork();
    if (pid == 0) {
        try {
            lxcpp::setns(::getpid(), {Namespace::MNT,
                                      Namespace::PID,
                                      Namespace::UTS,
                                      Namespace::IPC,
                                      Namespace::NET
                                     });
            ::_exit(TEST_PASSED);
        } catch(...) {
            ::_exit(ERROR);
        }
    } else if (pid >  0) {
        int status = -1;
        BOOST_REQUIRE(utils::waitPid(pid, status));
        BOOST_REQUIRE(status == TEST_PASSED);
    }
}

BOOST_AUTO_TEST_CASE(SetnsUserNamespace)
{
    const int TEST_PASSED = 0;
    const int ERROR = -1;

    pid_t pid = lxcpp::fork();
    if (pid == 0) {
        try {
            lxcpp::setns(::getpid(), {Namespace::USER});
            ::_exit(ERROR);
        } catch(ProcessSetupException) {
            ::_exit(TEST_PASSED);
        } catch(...) {
            ::_exit(ERROR);
        }
    } else if (pid > 0) {
        int status;
        BOOST_REQUIRE(utils::waitPid(pid, status));
        BOOST_REQUIRE(status == TEST_PASSED);
    }
}

BOOST_AUTO_TEST_SUITE_END()
