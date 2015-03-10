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
 * @author  Piotr Bartosiewicz (p.bartosiewi@partner.samsung.com)
 * @brief   Common unit tests include file
 */

#ifndef UNIT_TESTS_UT_HPP
#define UNIT_TESTS_UT_HPP

#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <boost/mpl/vector.hpp>

#include <string>

/**
 * Usage example:
 *
 * MULTI_FIXTURE_TEST_CASE(Test, T, Fixture1, Fixture2, Fixture3) {
 *     std::cout << T::i << "\n";
 * }
 */
#define MULTI_FIXTURE_TEST_CASE(NAME, TPARAM, ...) \
    typedef boost::mpl::vector<__VA_ARGS__> NAME##_fixtures; \
    BOOST_FIXTURE_TEST_CASE_TEMPLATE(NAME, TPARAM, NAME##_fixtures, TPARAM)


/**
 * An exception message checker
 *
 * Usage example:
 * BOOST_CHECK_EXCEPTION(foo(), SomeException, WhatEquals("oops"))
 */
class WhatEquals {
public:
    explicit WhatEquals(const std::string& message)
        : mMessage(message) {}

    template <typename T>
    bool operator()(const T& e)
    {
        BOOST_WARN_EQUAL(e.what(), mMessage); // additional failure info
        return e.what() == mMessage;
    }
private:
    std::string mMessage;
};

#endif // UNIT_TESTS_UT_HPP
