/*
 *  Copyright (C) 2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License version 2.1 as published by the Free Software Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
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
