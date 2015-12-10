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
 * @brief   Unit tests of counting map
 */

#include "config.hpp"

#include "ut.hpp"

#include "utils/counting-map.hpp"

BOOST_AUTO_TEST_SUITE(CountingMapSuite)

using namespace utils;

BOOST_AUTO_TEST_CASE(Counting)
{
    CountingMap<std::string> map;

    BOOST_CHECK(map.empty());
    BOOST_CHECK_EQUAL(0, map.get("ala"));

    BOOST_CHECK_EQUAL(1, map.increment("ala"));
    BOOST_CHECK_EQUAL(1, map.increment("ma"));

    BOOST_CHECK(!map.empty());
    BOOST_CHECK_EQUAL(1, map.get("ala"));
    BOOST_CHECK_EQUAL(1, map.get("ma"));
    BOOST_CHECK_EQUAL(0, map.get("kota"));

    BOOST_CHECK_EQUAL(2, map.increment("ala"));
    BOOST_CHECK_EQUAL(2, map.increment("ma"));
    BOOST_CHECK_EQUAL(3, map.increment("ma"));

    BOOST_CHECK(!map.empty());
    BOOST_CHECK_EQUAL(2, map.get("ala"));
    BOOST_CHECK_EQUAL(3, map.get("ma"));
    BOOST_CHECK_EQUAL(0, map.get("kota"));

    BOOST_CHECK_EQUAL(1, map.decrement("ala"));
    BOOST_CHECK_EQUAL(0, map.decrement("kota"));

    BOOST_CHECK(!map.empty());
    BOOST_CHECK_EQUAL(1, map.get("ala"));
    BOOST_CHECK_EQUAL(3, map.get("ma"));
    BOOST_CHECK_EQUAL(0, map.get("kota"));

    BOOST_CHECK_EQUAL(0, map.decrement("ala"));

    BOOST_CHECK(!map.empty());
    BOOST_CHECK_EQUAL(0, map.get("ala"));
    BOOST_CHECK_EQUAL(3, map.get("ma"));
    BOOST_CHECK_EQUAL(0, map.get("kota"));

    BOOST_CHECK_EQUAL(2, map.decrement("ma"));
    BOOST_CHECK_EQUAL(1, map.decrement("ma"));
    BOOST_CHECK_EQUAL(0, map.decrement("ma"));

    BOOST_CHECK(map.empty());
}

BOOST_AUTO_TEST_SUITE_END()
