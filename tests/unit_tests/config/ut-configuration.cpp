/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Michal Witanowski <m.witanowski@samsung.com>
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
 * @author  Michal Witanowski (m.witanowski@samsung.com)
 * @brief   Unit test of Configuration
 */

#include "config.hpp"
#include "ut.hpp"
#include "config/fields.hpp"
#include "config/manager.hpp"
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;
using namespace config;

BOOST_AUTO_TEST_SUITE(ConfigurationSuite)

struct TestConfig {
    // subtree class
    struct SubConfig {

        struct SubSubConfig {
            int intVal;

            CONFIG_REGISTER
            (
                intVal
            )
        };

        int intVal;
        SubSubConfig subSubObj;

        CONFIG_REGISTER
        (
            intVal,
            subSubObj
        )
    };

    int intVal;
    std::int64_t int64Val;
    std::string stringVal;
    double doubleVal;
    bool boolVal;

    std::vector<int> intVector;
    std::vector<std::string> stringVector;
    std::vector<double> doubleVector;

    SubConfig subObj;
    std::vector<SubConfig> subVector;

    CONFIG_REGISTER
    (
        intVal,
        int64Val,
        stringVal,
        doubleVal,
        boolVal,

        intVector,
        stringVector,
        doubleVector,

        subObj,
        subVector
    )
};

/**
 * JSON string used in ConfigSuite test cases
 * For the purpose of these tests the order of this string
 * has to be equal to the above REGISTER order
 */
const std::string jsonTestString =
    "{ \"intVal\": 12345, "
    "\"int64Val\": -1234567890123456789, "
    "\"stringVal\": \"blah\", "
    "\"doubleVal\": -1.234000, "
    "\"boolVal\": true, "
    "\"intVector\": [ 1, 2, 3 ], "
    "\"stringVector\": [ \"a\", \"b\" ], "
    "\"doubleVector\": [ 0.000000, 1.000000, 2.000000 ], "
    "\"subObj\": { \"intVal\": 54321, \"subSubObj\": { \"intVal\": 234 } }, "
    "\"subVector\": [ { \"intVal\": 123, \"subSubObj\": { \"intVal\": 345 } }, "
                     "{ \"intVal\": 456, \"subSubObj\": { \"intVal\": 567 } } ] }";

// Floating point tolerance as a number of rounding errors
const int TOLERANCE = 1;


BOOST_AUTO_TEST_CASE(FromStringTest)
{
    TestConfig testConfig;

    BOOST_REQUIRE_NO_THROW(loadFromString(jsonTestString, testConfig));

    BOOST_CHECK_EQUAL(12345, testConfig.intVal);
    BOOST_CHECK_EQUAL(-1234567890123456789ll, testConfig.int64Val);
    BOOST_CHECK_EQUAL("blah", testConfig.stringVal);
    BOOST_CHECK_CLOSE(-1.234, testConfig.doubleVal, TOLERANCE);
    BOOST_CHECK_EQUAL(true, testConfig.boolVal);

    BOOST_REQUIRE_EQUAL(3, testConfig.intVector.size());
    BOOST_CHECK_EQUAL(1, testConfig.intVector[0]);
    BOOST_CHECK_EQUAL(2, testConfig.intVector[1]);
    BOOST_CHECK_EQUAL(3, testConfig.intVector[2]);

    BOOST_REQUIRE_EQUAL(2, testConfig.stringVector.size());
    BOOST_CHECK_EQUAL("a", testConfig.stringVector[0]);
    BOOST_CHECK_EQUAL("b", testConfig.stringVector[1]);

    BOOST_REQUIRE_EQUAL(3, testConfig.doubleVector.size());
    BOOST_CHECK_CLOSE(0.0, testConfig.doubleVector[0], TOLERANCE);
    BOOST_CHECK_CLOSE(1.0, testConfig.doubleVector[1], TOLERANCE);
    BOOST_CHECK_CLOSE(2.0, testConfig.doubleVector[2], TOLERANCE);

    BOOST_CHECK_EQUAL(54321, testConfig.subObj.intVal);

    BOOST_REQUIRE_EQUAL(2, testConfig.subVector.size());
    BOOST_CHECK_EQUAL(123, testConfig.subVector[0].intVal);
    BOOST_CHECK_EQUAL(456, testConfig.subVector[1].intVal);
    BOOST_CHECK_EQUAL(345, testConfig.subVector[0].subSubObj.intVal);
    BOOST_CHECK_EQUAL(567, testConfig.subVector[1].subSubObj.intVal);
}


BOOST_AUTO_TEST_CASE(ToStringTest)
{
    TestConfig testConfig;
    BOOST_REQUIRE_NO_THROW(loadFromString(jsonTestString, testConfig));

    std::string out = saveToString(testConfig);
    BOOST_CHECK_EQUAL(out, jsonTestString);
}

namespace loadErrorsTest {

#define DECLARE_CONFIG(name, type) \
    struct name { \
        type field; \
        CONFIG_REGISTER(field) \
    };
DECLARE_CONFIG(IntConfig, int)
DECLARE_CONFIG(StringConfig, std::string)
DECLARE_CONFIG(DoubleConfig, double)
DECLARE_CONFIG(BoolConfig, bool)
DECLARE_CONFIG(ArrayConfig, std::vector<int>)
DECLARE_CONFIG(ObjectConfig, IntConfig)
#undef DECLARE_CONFIG

} // namespace loadErrorsTest

BOOST_AUTO_TEST_CASE(LoadErrorsTest)
{
    using namespace loadErrorsTest;

    IntConfig config;
    BOOST_REQUIRE_NO_THROW(loadFromString("{\"field\":1}", config));

    BOOST_CHECK_THROW(loadFromString("", config), ConfigException);
    BOOST_CHECK_THROW(loadFromString("{", config), ConfigException); // invalid json
    BOOST_CHECK_THROW(loadFromString("{}", config), ConfigException); // missing field

    // invalid type

    IntConfig intConfig;
    BOOST_CHECK_NO_THROW(loadFromString("{\"field\": 1}", intConfig));
    BOOST_CHECK_THROW(loadFromString("{\"field\": \"1\"}", intConfig), ConfigException);
    BOOST_CHECK_THROW(loadFromString("{\"field\": 1.0}", intConfig), ConfigException);
    BOOST_CHECK_THROW(loadFromString("{\"field\": true}", intConfig), ConfigException);
    BOOST_CHECK_THROW(loadFromString("{\"field\": []}", intConfig), ConfigException);
    BOOST_CHECK_THROW(loadFromString("{\"field\": {}}", intConfig), ConfigException);
    BOOST_CHECK_THROW(loadFromString("{\"field\": 1234567890123456789}", intConfig), ConfigException);
    BOOST_CHECK_THROW(loadFromString("{\"field\": -1234567890123456789}", intConfig), ConfigException);

    StringConfig stringConfig;
    BOOST_CHECK_THROW(loadFromString("{\"field\": 1}", stringConfig), ConfigException);
    BOOST_CHECK_NO_THROW(loadFromString("{\"field\": \"1\"}", stringConfig));
    BOOST_CHECK_THROW(loadFromString("{\"field\": 1.0}", stringConfig), ConfigException);
    BOOST_CHECK_THROW(loadFromString("{\"field\": true}", stringConfig), ConfigException);
    BOOST_CHECK_THROW(loadFromString("{\"field\": []}", stringConfig), ConfigException);
    BOOST_CHECK_THROW(loadFromString("{\"field\": {}}", stringConfig), ConfigException);

    DoubleConfig doubleConfig;
    BOOST_CHECK_THROW(loadFromString("{\"field\": 1}", doubleConfig), ConfigException);
    BOOST_CHECK_THROW(loadFromString("{\"field\": \"1\"}", doubleConfig), ConfigException);
    BOOST_CHECK_NO_THROW(loadFromString("{\"field\": 1.0}", doubleConfig));
    BOOST_CHECK_THROW(loadFromString("{\"field\": true}", doubleConfig), ConfigException);
    BOOST_CHECK_THROW(loadFromString("{\"field\": []}", doubleConfig), ConfigException);
    BOOST_CHECK_THROW(loadFromString("{\"field\": {}}", doubleConfig), ConfigException);

    BoolConfig boolConfig;
    BOOST_CHECK_THROW(loadFromString("{\"field\": 1}", boolConfig), ConfigException);
    BOOST_CHECK_THROW(loadFromString("{\"field\": \"1\"}", boolConfig), ConfigException);
    BOOST_CHECK_THROW(loadFromString("{\"field\": 1.0}", boolConfig), ConfigException);
    BOOST_CHECK_NO_THROW(loadFromString("{\"field\": true}", boolConfig));
    BOOST_CHECK_THROW(loadFromString("{\"field\": []}", boolConfig), ConfigException);
    BOOST_CHECK_THROW(loadFromString("{\"field\": {}}", boolConfig), ConfigException);

    ArrayConfig arrayConfig;
    BOOST_CHECK_THROW(loadFromString("{\"field\": 1}", arrayConfig), ConfigException);
    BOOST_CHECK_THROW(loadFromString("{\"field\": \"1\"}", arrayConfig), ConfigException);
    BOOST_CHECK_THROW(loadFromString("{\"field\": 1.0}", arrayConfig), ConfigException);
    BOOST_CHECK_THROW(loadFromString("{\"field\": true}", arrayConfig), ConfigException);
    BOOST_CHECK_NO_THROW(loadFromString("{\"field\": []}", arrayConfig));
    BOOST_CHECK_THROW(loadFromString("{\"field\": {}}", arrayConfig), ConfigException);

    ObjectConfig objectConfig;
    BOOST_CHECK_THROW(loadFromString("{\"field\": 1}", objectConfig), ConfigException);
    BOOST_CHECK_THROW(loadFromString("{\"field\": \"1\"}", objectConfig), ConfigException);
    BOOST_CHECK_THROW(loadFromString("{\"field\": 1.0}", objectConfig), ConfigException);
    BOOST_CHECK_THROW(loadFromString("{\"field\": true}", objectConfig), ConfigException);
    BOOST_CHECK_THROW(loadFromString("{\"field\": []}", objectConfig), ConfigException);
    BOOST_CHECK_THROW(loadFromString("{\"field\": {}}", objectConfig), ConfigException);
    BOOST_CHECK_NO_THROW(loadFromString("{\"field\": {\"field\": 1}}", objectConfig));
}

namespace hasVisitableTest {

struct NotVisitable {};
struct Visitable {
    template<typename V>
    void accept(V v);
};
struct ConstVisitable {
    template<typename V>
    void accept(V v) const;
};
struct FullVisitable {
    template<typename V>
    void accept(V v);
    template<typename V>
    void accept(V v) const;
};
struct DerivedVisitable : FullVisitable {};
struct MissingArg {
    template<typename V>
    void accept();
};
struct WrongArg {
    template<typename V>
    void accept(int v);
};
struct NotFunction {
    int accept;
};

} // namespace hasVisitableTest

BOOST_AUTO_TEST_CASE(HasVisibleInternalHelperTest)
{
    using namespace hasVisitableTest;

    static_assert(isVisitable<Visitable>::value, "");
    static_assert(isVisitable<ConstVisitable>::value, "");
    static_assert(isVisitable<FullVisitable>::value, "");
    static_assert(isVisitable<DerivedVisitable>::value, "");

    static_assert(!isVisitable<NotVisitable>::value, "");
    static_assert(!isVisitable<MissingArg>::value, "");
    static_assert(!isVisitable<WrongArg>::value, "");
    static_assert(!isVisitable<NotFunction>::value, "");

    BOOST_CHECK(isVisitable<Visitable>());
}

namespace saveLoadKVStoreTest {

// This struct is like TestConfig, but without a list of structures.
struct PoorTestConfig {
    // subtree class
    struct SubConfig {

        struct SubSubConfig {
            int intVal;

            CONFIG_REGISTER
            (
                intVal
            )
        };

        int intVal;
        SubSubConfig subSubObj;

        CONFIG_REGISTER
        (
            intVal,
            subSubObj
        )
    };

    int intVal;
    std::int64_t int64Val;
    std::string stringVal;
    double doubleVal;
    bool boolVal;

    std::vector<int> intVector;
    std::vector<std::string> stringVector;
    std::vector<double> doubleVector;

    SubConfig subObj;

    CONFIG_REGISTER
    (
        intVal,
        int64Val,
        stringVal,
        doubleVal,
        boolVal,

        intVector,
        stringVector,
        doubleVector,

        subObj
    )
};
} // saveLoadKVStoreTest


BOOST_AUTO_TEST_CASE(FromToKVStoreTest)
{
    using namespace saveLoadKVStoreTest;

    // TODO: Change this to TestConfig and delete PoorTestConfig when serialization is implemented
    PoorTestConfig config;
    loadFromString(jsonTestString, config);

    std::string dbPath = fs::unique_path("/tmp/kvstore-%%%%.db3").string();

    saveToKVStore(dbPath, config);
    loadFromKVStore(dbPath, config);
    saveToKVStore(dbPath, config, "some_config");
    loadFromKVStore(dbPath, config, "some_config");

    fs::remove(dbPath);
}

BOOST_AUTO_TEST_SUITE_END()
