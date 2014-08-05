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
 * @brief   Unit test of KVStore class
 */

#include "config.hpp"
#include "ut.hpp"

#include "config/kvstore.hpp"
#include "config/exception.hpp"

#include <memory>
#include <boost/filesystem.hpp>

using namespace config;
namespace fs = boost::filesystem;

namespace {

struct Fixture {
    std::string dbPath;
    KVStore c;

    Fixture()
        : dbPath(fs::unique_path("/tmp/kvstore-%%%%.db3").string()),
          c(dbPath)
    {
    }
    ~Fixture()
    {
        fs::remove(dbPath);
    }
};
} // namespace

BOOST_FIXTURE_TEST_SUITE(KVStoreSuite, Fixture)

const std::string KEY = "KEY";

BOOST_AUTO_TEST_CASE(SimpleConstructorDestructorTest)
{
    const std::string dbPath =  fs::unique_path("/tmp/kvstore-%%%%.db3").string();
    std::unique_ptr<KVStore> conPtr;
    BOOST_REQUIRE_NO_THROW(conPtr.reset(new KVStore(dbPath)));
    BOOST_CHECK(fs::exists(dbPath));
    BOOST_REQUIRE_NO_THROW(conPtr.reset(new KVStore(dbPath)));
    BOOST_CHECK(fs::exists(dbPath));
    BOOST_REQUIRE_NO_THROW(conPtr.reset());
    BOOST_CHECK(fs::exists(dbPath));
    fs::remove(dbPath);
}

BOOST_AUTO_TEST_CASE(SingleValueTest)
{
    // Set
    BOOST_CHECK_NO_THROW(c.set(KEY, "A"));
    BOOST_CHECK_EQUAL(c.get(KEY), "A");

    // Update
    BOOST_CHECK_NO_THROW(c.set(KEY, "B"));
    BOOST_CHECK_EQUAL(c.get(KEY), "B");
    BOOST_CHECK_EQUAL(c.count(KEY), 1);

    // Remove
    BOOST_CHECK_NO_THROW(c.remove(KEY));
    BOOST_CHECK_EQUAL(c.count(KEY), 0);
    BOOST_CHECK_THROW(c.get(KEY), ConfigException);
}

BOOST_AUTO_TEST_CASE(EscapedCharactersTest)
{
    // '*' ?' '[' ']' are escaped
    // They shouldn't influence the internal implementation
    std::string HARD_KEY = "[" + KEY;
    BOOST_CHECK_NO_THROW(c.set(HARD_KEY, "A"));
    BOOST_CHECK_NO_THROW(c.set(KEY, "B"));
    BOOST_CHECK_EQUAL(c.count(HARD_KEY), 1);
    BOOST_CHECK_EQUAL(c.count(KEY), 1);
    BOOST_CHECK_EQUAL(c.size(), 2);
    BOOST_CHECK_NO_THROW(c.clear());

    HARD_KEY = "]" + KEY;
    BOOST_CHECK_NO_THROW(c.set(HARD_KEY, "A"));
    BOOST_CHECK_NO_THROW(c.set(KEY, "B"));
    BOOST_CHECK_EQUAL(c.count(HARD_KEY), 1);
    BOOST_CHECK_EQUAL(c.count(KEY), 1);
    BOOST_CHECK_EQUAL(c.size(), 2);
    BOOST_CHECK_NO_THROW(c.clear());

    HARD_KEY = "?" + KEY;
    BOOST_CHECK_NO_THROW(c.set(HARD_KEY, "A"));
    BOOST_CHECK_NO_THROW(c.set(KEY, "B"));
    BOOST_CHECK_EQUAL(c.count(HARD_KEY), 1);
    BOOST_CHECK_EQUAL(c.count(KEY), 1);
    BOOST_CHECK_EQUAL(c.size(), 2);
    BOOST_CHECK_NO_THROW(c.clear());

    HARD_KEY = "*" + KEY;
    BOOST_CHECK_NO_THROW(c.set(HARD_KEY, "A"));
    BOOST_CHECK_NO_THROW(c.set(KEY, "B"));
    BOOST_CHECK_EQUAL(c.count(HARD_KEY), 1);
    BOOST_CHECK_EQUAL(c.count(KEY), 1);
    BOOST_CHECK_EQUAL(c.size(), 2);
}

BOOST_AUTO_TEST_CASE(VectorOfValuesTest)
{
    std::vector<std::string> AB = {"A", "B"};
    std::vector<std::string> AC = {"A", "C"};
    std::vector<std::string> ABC = {"A", "B", "C"};

    // Set
    BOOST_CHECK_NO_THROW(c.set(KEY, AB));
    BOOST_CHECK(c.list(KEY) == AB);
    BOOST_CHECK_EQUAL(c.count(KEY), 2);
    BOOST_CHECK_EQUAL(c.size(), 2);


    // Update
    BOOST_CHECK_NO_THROW(c.set(KEY, AC));
    BOOST_CHECK(c.list(KEY) == AC);
    BOOST_CHECK_EQUAL(c.count(KEY), 2);
    BOOST_CHECK_EQUAL(c.size(), 2);

    // Update
    BOOST_CHECK_NO_THROW(c.set(KEY, ABC));
    BOOST_CHECK(c.list(KEY) == ABC);
    BOOST_CHECK_EQUAL(c.count(KEY), 3);
    BOOST_CHECK_EQUAL(c.size(), 3);

    // Update
    BOOST_CHECK_NO_THROW(c.set(KEY, AC));
    BOOST_CHECK(c.list(KEY) == AC);
    BOOST_CHECK_EQUAL(c.count(KEY), 2);
    BOOST_CHECK_EQUAL(c.size(), 2);

    // Remove
    BOOST_CHECK_NO_THROW(c.remove(KEY));
    BOOST_CHECK_EQUAL(c.count(KEY), 0);
    BOOST_CHECK_EQUAL(c.size(), 0);
    BOOST_CHECK_THROW(c.list(KEY), ConfigException);
    BOOST_CHECK_THROW(c.get(KEY), ConfigException);
}

BOOST_AUTO_TEST_CASE(ClearTest)
{
    BOOST_CHECK_NO_THROW(c.clear());

    BOOST_CHECK_NO_THROW(c.set(KEY, {"A", "B"}));
    BOOST_CHECK_NO_THROW(c.clear());
    BOOST_CHECK_EQUAL(c.size(), 0);

    BOOST_CHECK_NO_THROW(c.remove(KEY));
    BOOST_CHECK_THROW(c.list(KEY), ConfigException);
    BOOST_CHECK_THROW(c.get(KEY), ConfigException);
}

BOOST_AUTO_TEST_SUITE_END()
