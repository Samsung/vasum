/*
 *  Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Krzysztof Dynowski (k.dynowski@samsumg.com)
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
 * @author  Krzysztof Dynowski (k.dynowski@samsung.com)
 * @brief   Unit tests of text helpers
 */

#include "config.hpp"

#include "ut.hpp"

#include "utils/text.hpp"

BOOST_AUTO_TEST_SUITE(TextUtilsSuite)

BOOST_AUTO_TEST_CASE(SplitText)
{
    std::vector<std::string> v;
    v = utils::split("", ",");
    BOOST_CHECK(v.size() == 0);
    v = utils::split("a", ",");
    BOOST_CHECK(v.size() == 1);
    v = utils::split(",", ",");
    BOOST_CHECK(v.size() == 2);
    v = utils::split("1,2", ",");
    BOOST_CHECK(v.size() == 2);
}

BOOST_AUTO_TEST_SUITE_END()
