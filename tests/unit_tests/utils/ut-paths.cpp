/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Jan Olszak <j.olszak@samsung.com>
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
 * @brief   Unit tests of utils
 */

#include "config.hpp"

#include "ut.hpp"

#include "utils/paths.hpp"

#include <memory>

BOOST_AUTO_TEST_SUITE(UtilsPathsSuite)

using namespace utils;

BOOST_AUTO_TEST_CASE(CreateFilePath)
{
    BOOST_CHECK_EQUAL("", createFilePath());

    BOOST_CHECK_EQUAL("a", createFilePath("a"));
    BOOST_CHECK_EQUAL("/", createFilePath("/"));

    BOOST_CHECK_EQUAL("", createFilePath("", ""));
    BOOST_CHECK_EQUAL("a", createFilePath("a", ""));
    BOOST_CHECK_EQUAL("b", createFilePath("", "b"));
    BOOST_CHECK_EQUAL("/", createFilePath("", "/"));
    BOOST_CHECK_EQUAL("/", createFilePath("/", ""));
    BOOST_CHECK_EQUAL("/", createFilePath("/", "/"));

    BOOST_CHECK_EQUAL("a/b", createFilePath("a", "b"));
    BOOST_CHECK_EQUAL("a/b", createFilePath("a/", "b"));
    BOOST_CHECK_EQUAL("a/b", createFilePath("a", "/b"));
    BOOST_CHECK_EQUAL("a/b", createFilePath("a/", "/b"));

    BOOST_CHECK_EQUAL("a/b.txt",  createFilePath("a", "b", ".txt"));
    BOOST_CHECK_EQUAL("a/b.txt",  createFilePath("a/", "b", ".txt"));
    BOOST_CHECK_EQUAL("a/b.txt",  createFilePath("a", "/b", ".txt"));
    BOOST_CHECK_EQUAL("a/b/.txt", createFilePath("a", "/b", "/.txt"));
    BOOST_CHECK_EQUAL("a/b/.txt", createFilePath("a", "/b/", "/.txt"));
}

BOOST_AUTO_TEST_CASE(DirName)
{
    BOOST_CHECK_EQUAL(".", dirName(""));
    BOOST_CHECK_EQUAL(".", dirName("."));
    BOOST_CHECK_EQUAL(".", dirName("./"));
    BOOST_CHECK_EQUAL(".", dirName(".///"));
    BOOST_CHECK_EQUAL("/", dirName("/"));
    BOOST_CHECK_EQUAL("/", dirName("///"));

    BOOST_CHECK_EQUAL("/", dirName("/level1"));
    BOOST_CHECK_EQUAL("/", dirName("/level1/"));
    BOOST_CHECK_EQUAL("/level1", dirName("/level1/level2"));
    BOOST_CHECK_EQUAL("/level1", dirName("/level1/level2/"));
    BOOST_CHECK_EQUAL("/level1/level2", dirName("/level1/level2/level3"));
    BOOST_CHECK_EQUAL("/level1/level2", dirName("/level1/level2/level3/"));

    BOOST_CHECK_EQUAL(".", dirName("level1"));
    BOOST_CHECK_EQUAL(".", dirName("level1/"));
    BOOST_CHECK_EQUAL("level1", dirName("level1/level2"));
    BOOST_CHECK_EQUAL("level1", dirName("level1/level2"));
    BOOST_CHECK_EQUAL("level1", dirName("level1/level2/"));
    BOOST_CHECK_EQUAL("level1/level2", dirName("level1/level2/level3"));
    BOOST_CHECK_EQUAL("level1/level2", dirName("level1/level2/level3/"));

    BOOST_CHECK_EQUAL(".", dirName("./level1"));
    BOOST_CHECK_EQUAL(".", dirName("./level1/"));
    BOOST_CHECK_EQUAL("./level1", dirName("./level1/level2"));
    BOOST_CHECK_EQUAL("./level1", dirName("./level1/level2/"));
    BOOST_CHECK_EQUAL("./level1/level2", dirName("./level1/level2/level3"));
    BOOST_CHECK_EQUAL("./level1/level2", dirName("./level1/level2/level3/"));

    BOOST_CHECK_EQUAL(".", dirName(".."));
    BOOST_CHECK_EQUAL(".", dirName("../"));
    BOOST_CHECK_EQUAL("..", dirName("../level1"));
    BOOST_CHECK_EQUAL("..", dirName("../level1/"));
    BOOST_CHECK_EQUAL("../level1", dirName("../level1/level2"));
    BOOST_CHECK_EQUAL("../level1", dirName("../level1/level2/"));

    BOOST_CHECK_EQUAL("/", dirName("/.."));
    BOOST_CHECK_EQUAL("/", dirName("/../"));
    BOOST_CHECK_EQUAL("/level1", dirName("/level1/.."));
    BOOST_CHECK_EQUAL("/level1", dirName("/level1/../"));
    BOOST_CHECK_EQUAL("/level1/..", dirName("/level1/../level2"));
    BOOST_CHECK_EQUAL("/level1/..", dirName("/level1/../level2/"));

    BOOST_CHECK_EQUAL("/", dirName("///.."));
    BOOST_CHECK_EQUAL("/", dirName("//..///"));
    BOOST_CHECK_EQUAL("/level1", dirName("//level1//.."));
    BOOST_CHECK_EQUAL("/level1", dirName("//level1//..///"));
    BOOST_CHECK_EQUAL("/level1/..", dirName("//level1////..//level2"));
    BOOST_CHECK_EQUAL("/level1/..", dirName("////level1//..////level2///"));
}

BOOST_AUTO_TEST_SUITE_END()
