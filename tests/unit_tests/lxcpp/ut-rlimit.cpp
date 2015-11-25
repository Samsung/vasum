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
 * @brief   Unit tests of lxcpp rlimit helpers
 */

#include "config.hpp"
#include "ut.hpp"

#include "lxcpp/rlimit.hpp"
#include "lxcpp/process.hpp"

namespace {

const int RLIMIT_TYPE = RLIMIT_CPU;
const int WRONG_RLIMIT_TYPE = -1;
const uint64_t SOFT_LIMIT = 1024;
const uint64_t HARD_LIMIT = 102400;

int clonefn(void* _limit)
{
    uint64_t limit = (uint64_t)_limit;

    try {
        lxcpp::setRlimit(RLIMIT_TYPE, SOFT_LIMIT, HARD_LIMIT);
        lxcpp::setRlimit(RLIMIT_TYPE, SOFT_LIMIT, (HARD_LIMIT + limit));

        struct rlimit rlim = lxcpp::getRlimit(RLIMIT_TYPE);

        if(rlim.rlim_cur == SOFT_LIMIT && rlim.rlim_max == (HARD_LIMIT + limit)) {
            ::_exit(EXIT_SUCCESS);
        }
    } catch(...) {}
    ::_exit(EXIT_FAILURE);

    return 0;
}

} // namespace

BOOST_AUTO_TEST_SUITE(LxcppRlimitSuite)

using namespace lxcpp;

BOOST_AUTO_TEST_CASE(SetGetRlimit)
{
    pid_t pid = lxcpp::fork();
    if (pid == 0) {
        try {

            lxcpp::setRlimit(RLIMIT_TYPE, SOFT_LIMIT, HARD_LIMIT);

            struct rlimit rlim = lxcpp::getRlimit(RLIMIT_TYPE);

            if(rlim.rlim_cur == SOFT_LIMIT && rlim.rlim_max == HARD_LIMIT) {
                ::_exit(EXIT_SUCCESS);
            }
        } catch(...) {}
        ::_exit(EXIT_FAILURE);
    }
    BOOST_REQUIRE(lxcpp::waitpid(pid) == EXIT_SUCCESS);
}

BOOST_AUTO_TEST_CASE(SetWrongRlimit)
{
    pid_t pid = lxcpp::fork();
    if (pid == 0) {
        try {
            lxcpp::setRlimit(WRONG_RLIMIT_TYPE, SOFT_LIMIT, HARD_LIMIT);
            ::_exit(EXIT_SUCCESS);
        } catch(...) {
            ::_exit(EXIT_FAILURE);
        }
    }
    BOOST_REQUIRE(lxcpp::waitpid(pid) == EXIT_FAILURE);
}

BOOST_AUTO_TEST_CASE(SetGetRlimitInUserNS)
{
    // Unprivileged process may only reduce hardlimit
    pid_t pid = lxcpp::clone(clonefn, (void*)(-10), CLONE_NEWUSER);
    BOOST_REQUIRE(lxcpp::waitpid(pid) == EXIT_SUCCESS);

    pid = lxcpp::clone(clonefn, (void*)(+10), CLONE_NEWUSER);
    BOOST_REQUIRE(lxcpp::waitpid(pid) == EXIT_FAILURE);
}

BOOST_AUTO_TEST_SUITE_END()
