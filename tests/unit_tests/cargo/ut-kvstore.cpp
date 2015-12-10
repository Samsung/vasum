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

#include "cargo-sqlite/internals/kvstore.hpp"
#include "cargo/exception.hpp"
#include "utils/scoped-dir.hpp"
#include "utils/latch.hpp"

#include <iostream>
#include <memory>
#include <thread>
#include <boost/filesystem.hpp>

using namespace cargo;
using namespace utils;
using namespace cargo::internals;
namespace fs = boost::filesystem;

namespace {

const std::string UT_PATH = "/tmp/ut-config/";

struct Fixture {
    ScopedDir mUTDirGuard;
    std::string dbPath;
    KVStore c;

    Fixture()
        : mUTDirGuard(UT_PATH)
        , dbPath(UT_PATH + "kvstore.db3")
        , c(dbPath)
    {
    }
};

} // namespace

BOOST_FIXTURE_TEST_SUITE(KVStoreSuite, Fixture)

const std::string KEY = "KEY";

BOOST_AUTO_TEST_CASE(SimpleConstructorDestructor)
{
    std::unique_ptr<KVStore> conPtr;
    BOOST_REQUIRE_NO_THROW(conPtr.reset(new KVStore(dbPath)));
    BOOST_CHECK(fs::exists(dbPath));
    BOOST_REQUIRE_NO_THROW(conPtr.reset(new KVStore(dbPath)));
    BOOST_CHECK(fs::exists(dbPath));
    BOOST_REQUIRE_NO_THROW(conPtr.reset());
    BOOST_CHECK(fs::exists(dbPath));
}

BOOST_AUTO_TEST_CASE(EscapedCharacters)
{
    // '*' ?' '[' ']' are escaped
    // They shouldn't influence the internal implementation
    for (char sc: {'[', ']', '?', '*'}) {
        std::string HARD_KEY = sc + KEY;

        BOOST_CHECK_NO_THROW(c.set(HARD_KEY, "A"));
        BOOST_CHECK_NO_THROW(c.set(KEY, "B"));

        BOOST_CHECK(c.exists(HARD_KEY));
        BOOST_CHECK(c.exists(KEY));

        BOOST_CHECK_NO_THROW(c.clear());
    }
}

BOOST_AUTO_TEST_CASE(PrefixExists)
{
    // '*' ?' '[' ']' are escaped
    // They shouldn't influence the internal implementation
    for (char sc: {'[', ']', '?', '*'}) {
        std::string HARD_KEY = sc + KEY;
        std::string FIELD_HARD_KEY = HARD_KEY + ".field";

        BOOST_CHECK_NO_THROW(c.set(FIELD_HARD_KEY, "C"));

        BOOST_CHECK(!c.exists(KEY));
        BOOST_CHECK(!c.exists(HARD_KEY));
        BOOST_CHECK(c.exists(FIELD_HARD_KEY));

        BOOST_CHECK(!c.prefixExists(KEY));
        BOOST_CHECK(c.prefixExists(HARD_KEY));
        BOOST_CHECK(c.prefixExists(FIELD_HARD_KEY));

        BOOST_CHECK_NO_THROW(c.clear());
    }
}

namespace {
void testSingleValue(Fixture& f, const std::string& a, const std::string& b)
{
    // Set
    BOOST_CHECK_NO_THROW(f.c.set(KEY, a));
    BOOST_CHECK_EQUAL(f.c.get(KEY), a);

    // Update
    BOOST_CHECK_NO_THROW(f.c.set(KEY, b));
    BOOST_CHECK_EQUAL(f.c.get(KEY), b);
    BOOST_CHECK(f.c.exists(KEY));

    // Remove
    BOOST_CHECK_NO_THROW(f.c.remove(KEY));
    BOOST_CHECK(!f.c.exists(KEY));
    BOOST_CHECK_THROW(f.c.get(KEY), CargoException);
}
} // namespace


BOOST_AUTO_TEST_CASE(SingleValue)
{
    testSingleValue(*this, "A", "B");
}

BOOST_AUTO_TEST_CASE(Clear)
{
    BOOST_CHECK_NO_THROW(c.clear());
    BOOST_CHECK_NO_THROW(c.set(KEY, "2"));
    BOOST_CHECK_NO_THROW(c.set(KEY + ".0", "A"));
    BOOST_CHECK_NO_THROW(c.set(KEY + ".1", "B"));
    BOOST_CHECK_NO_THROW(c.clear());
    BOOST_CHECK(c.isEmpty());

    BOOST_CHECK_NO_THROW(c.remove(KEY));
    BOOST_CHECK_THROW(c.get(KEY), CargoException);
    BOOST_CHECK_THROW(c.get(KEY), CargoException);
}

BOOST_AUTO_TEST_CASE(Transaction)
{
    {
        KVStore::Transaction trans(c);
        c.set(KEY, "a");
        trans.commit();
    }
    BOOST_CHECK_EQUAL(c.get(KEY), "a");

    {
        KVStore::Transaction trans(c);
        c.set(KEY, "b");
        // no commit
    }
    BOOST_CHECK_EQUAL(c.get(KEY), "a");

    {
        KVStore::Transaction trans(c);
        trans.commit();
        BOOST_CHECK_THROW(trans.commit(), CargoException);
        BOOST_CHECK_THROW(KVStore::Transaction{c}, CargoException);
    }
}

BOOST_AUTO_TEST_CASE(TransactionStacked)
{
    {
        KVStore::Transaction transOuter(c);
        KVStore::Transaction transInner(c);
    }

    {
        KVStore::Transaction transOuter(c);
        {
            KVStore::Transaction transInner(c);
            c.set(KEY, "a");
            // no inner commit
        }
        transOuter.commit();
    }
    BOOST_CHECK_EQUAL(c.get(KEY), "a");

    {
        KVStore::Transaction transOuter(c);
        {
            KVStore::Transaction transInner(c);
            c.set(KEY, "b");
            transInner.commit();
        }
        // no outer commit
    }
    BOOST_CHECK_EQUAL(c.get(KEY), "a");

    {
        KVStore::Transaction transOuter(c);
        KVStore::Transaction transInner(c);
        transOuter.commit();
        BOOST_CHECK_THROW(transInner.commit(), CargoException);
    }
}

BOOST_AUTO_TEST_CASE(TransactionThreads)
{
    Latch trans1Started, trans1Release, trans2Released;
    std::thread thread1([&] {
        KVStore::Transaction trans1(c);
        trans1Started.set();
        trans1Release.wait();
    });
    std::thread thread2([&] {
        trans1Started.wait();
        KVStore::Transaction trans2(c);
        trans2Released.set();
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    BOOST_CHECK(trans2Released.empty());
    trans1Release.set();
    thread1.join();
    trans2Released.wait();
    thread2.join();
}

BOOST_AUTO_TEST_SUITE_END()
