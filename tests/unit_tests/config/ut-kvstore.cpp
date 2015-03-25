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
#include "utils/scoped-dir.hpp"
#include "utils/latch.hpp"

#include <iostream>
#include <memory>
#include <thread>
#include <boost/filesystem.hpp>

using namespace config;
using namespace vasum::utils;
namespace fs = boost::filesystem;

namespace {

const std::string UT_PATH = "/tmp/ut-config/";

struct Fixture {
    vasum::utils::ScopedDir mUTDirGuard;
    std::string dbPath;
    KVStore c;

    Fixture()
        : mUTDirGuard(UT_PATH)
        , dbPath(UT_PATH + "kvstore.db3")
        , c(dbPath)
    {
    }
};

class TestClass {
public:
    TestClass(int v): value(v) {}
    TestClass(): value(0) {}
    friend std::ostream& operator<< (std::ostream& out, const TestClass& cPoint);
    friend std::istream& operator>> (std::istream& in, TestClass& cPoint);
    friend bool operator== (const TestClass& lhs, const TestClass& rhs);
    friend bool operator!= (const TestClass& lhs, const TestClass& rhs);

private:
    int value ;
};

bool operator==(const TestClass& lhs, const TestClass& rhs)
{
    return lhs.value == rhs.value;
}

bool operator!=(const TestClass& lhs, const TestClass& rhs)
{
    return lhs.value != rhs.value;
}

std::ostream& operator<< (std::ostream& out, const TestClass& tc)
{
    out << tc.value;;
    return out;
}

std::istream& operator>> (std::istream& in, TestClass& tc)
{
    in >> tc.value;
    return in;
}

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
    std::string HARD_KEY = "[" + KEY;
    BOOST_CHECK_NO_THROW(c.set(HARD_KEY, "A"));
    BOOST_CHECK_NO_THROW(c.set(KEY, "B"));
    BOOST_CHECK(c.exists(HARD_KEY));
    BOOST_CHECK(c.exists(KEY));
    BOOST_CHECK_NO_THROW(c.clear());

    HARD_KEY = "]" + KEY;
    BOOST_CHECK_NO_THROW(c.set(HARD_KEY, "A"));
    BOOST_CHECK_NO_THROW(c.set(KEY, "B"));
    BOOST_CHECK(c.exists(HARD_KEY));
    BOOST_CHECK(c.exists(KEY));
    BOOST_CHECK_NO_THROW(c.clear());

    HARD_KEY = "?" + KEY;
    BOOST_CHECK_NO_THROW(c.set(HARD_KEY, "A"));
    BOOST_CHECK_NO_THROW(c.set(KEY, "B"));
    BOOST_CHECK(c.exists(HARD_KEY));
    BOOST_CHECK(c.exists(KEY));
    BOOST_CHECK_NO_THROW(c.clear());

    HARD_KEY = "*" + KEY;
    BOOST_CHECK_NO_THROW(c.set(HARD_KEY, "A"));
    BOOST_CHECK_NO_THROW(c.set(KEY, "B"));
    BOOST_CHECK(c.exists(HARD_KEY));
    BOOST_CHECK(c.exists(KEY));
}

namespace {
template<typename A, typename B>
void testSingleValue(Fixture& f, const A& a, const B& b)
{
    // Set
    BOOST_CHECK_NO_THROW(f.c.set(KEY, a));
    BOOST_CHECK_EQUAL(f.c.get<A>(KEY), a);

    // Update
    BOOST_CHECK_NO_THROW(f.c.set(KEY, b));
    BOOST_CHECK_EQUAL(f.c.get<B>(KEY), b);
    BOOST_CHECK(f.c.exists(KEY));

    // Remove
    BOOST_CHECK_NO_THROW(f.c.remove(KEY));
    BOOST_CHECK(!f.c.exists(KEY));
    BOOST_CHECK_THROW(f.c.get<B>(KEY), ConfigException);
}
} // namespace


BOOST_AUTO_TEST_CASE(SingleValue)
{
    testSingleValue<std::string, std::string>(*this, "A", "B");
    testSingleValue<int, int>(*this, 1, 2);
    testSingleValue<double, double>(*this, 1.1, 2.2);
    testSingleValue<int, std::string>(*this, 2, "A");
    testSingleValue<int64_t, int64_t>(*this, INT64_MAX, INT64_MAX - 2);
    testSingleValue<TestClass, int>(*this, 11, 22);
}

namespace {
template<typename T>
void setVector(Fixture& f, const std::vector<T>& vec)
{
    std::vector<T> storedVec;
    BOOST_CHECK_NO_THROW(f.c.set(KEY, vec));
    BOOST_CHECK_NO_THROW(storedVec = f.c.get<std::vector<T>>(KEY))
    BOOST_CHECK_EQUAL_COLLECTIONS(storedVec.begin(), storedVec.end(), vec.begin(), vec.end());
}

template<typename T>
void testVectorOfValues(Fixture& f,
                        const std::vector<T>& a,
                        const std::vector<T>& b,
                        const std::vector<T>& c)
{
    // Set
    setVector(f, a);
    setVector(f, b);
    setVector(f, c);

    // Remove
    BOOST_CHECK_NO_THROW(f.c.remove(KEY));
    BOOST_CHECK(!f.c.exists(KEY));
    BOOST_CHECK(f.c.isEmpty());
    BOOST_CHECK_THROW(f.c.get<std::vector<T> >(KEY), ConfigException);
    BOOST_CHECK_THROW(f.c.get(KEY), ConfigException);
}
} // namespace

BOOST_AUTO_TEST_CASE(VectorOfValues)
{
    testVectorOfValues<std::string>(*this, {"A", "B"}, {"A", "C"}, {"A", "B", "C"});
    testVectorOfValues<int>(*this, {1, 2}, {1, 3}, {1, 2, 3});
    testVectorOfValues<int64_t>(*this, {INT64_MAX, 2}, {1, 3}, {INT64_MAX, 2, INT64_MAX});
    testVectorOfValues<double>(*this, {1.1, 2.2}, {1.1, 3.3}, {1.1, 2.2, 3.3});
    testVectorOfValues<TestClass>(*this, {1, 2}, {1, 3}, {1, 2, 3});
}

BOOST_AUTO_TEST_CASE(Clear)
{
    BOOST_CHECK_NO_THROW(c.clear());
    std::vector<std::string> vec = {"A", "B"};
    BOOST_CHECK_NO_THROW(c.set(KEY, vec));
    BOOST_CHECK_NO_THROW(c.clear());
    BOOST_CHECK(c.isEmpty());

    BOOST_CHECK_NO_THROW(c.remove(KEY));
    BOOST_CHECK_THROW(c.get<std::vector<std::string>>(KEY), ConfigException);
    BOOST_CHECK_THROW(c.get(KEY), ConfigException);
}

BOOST_AUTO_TEST_CASE(Transaction)
{
    {
        KVStore::Transaction trans(c);
        c.set<int>(KEY, 1);
        trans.commit();
    }
    BOOST_CHECK_EQUAL(c.get<int>(KEY), 1);

    {
        KVStore::Transaction trans(c);
        c.set<int>(KEY, 2);
        // no commit
    }
    BOOST_CHECK_EQUAL(c.get<int>(KEY), 1);

    {
        KVStore::Transaction trans(c);
        trans.commit();
        BOOST_CHECK_THROW(trans.commit(), ConfigException);
        BOOST_CHECK_THROW(KVStore::Transaction{c}, ConfigException);
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
            c.set<int>(KEY, 1);
            // no inner commit
        }
        transOuter.commit();
    }
    BOOST_CHECK_EQUAL(c.get<int>(KEY), 1);

    {
        KVStore::Transaction transOuter(c);
        {
            KVStore::Transaction transInner(c);
            c.set<int>(KEY, 2);
            transInner.commit();
        }
        // no outer commit
    }
    BOOST_CHECK_EQUAL(c.get<int>(KEY), 1);

    {
        KVStore::Transaction transOuter(c);
        KVStore::Transaction transInner(c);
        transOuter.commit();
        BOOST_CHECK_THROW(transInner.commit(), ConfigException);
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

BOOST_AUTO_TEST_CASE(Key)
{
    BOOST_CHECK_EQUAL(key(), "");
    BOOST_CHECK_EQUAL(key<>(), "");
    BOOST_CHECK_EQUAL(key(""), "");
    BOOST_CHECK_EQUAL(key("KEY"), "KEY");
    BOOST_CHECK_EQUAL(key<>("KEY"), "KEY");
    BOOST_CHECK_EQUAL(key("KEY", "A"), "KEY.A");
    BOOST_CHECK_EQUAL(key("KEY", 1, 2.2), "KEY.1.2.2");
    BOOST_CHECK_EQUAL(key("KEY", 1, "B"), "KEY.1.B");
    BOOST_CHECK_EQUAL(key<'_'>("KEY", 1, 2.2), "KEY_1_2.2");
}

BOOST_AUTO_TEST_SUITE_END()
