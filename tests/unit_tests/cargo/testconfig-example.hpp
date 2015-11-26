/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
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
 * @author  Krzysztof Dynowski (k.dynowski@samsumg.com)
 * @brief   Test configuration struct to be used in unit tests
 */

#ifndef TESTCONFIG_EXAMPLE_HPP
#define TESTCONFIG_EXAMPLE_HPP

#include <array>
#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <tuple>

#include "cargo/fields.hpp"
#include "cargo/fields-union.hpp"
#include "cargo-validator/validator.hpp"

enum class TestEnum: int {
    FIRST = 0,
    SECOND = 12,
    THIRD = 13
};

struct TestMapStruct
{
    std::string type;
    std::string source;
    std::vector<std::string> options;

    bool operator==(const TestMapStruct& m) const
    {
        return ((m.type == type) && (m.source == source) && (m.options == options));
    }

    CARGO_REGISTER
    (
        type,
        source,
        options
    )
};

struct TestConfig {
    // subtree class
    struct SubConfig {

        struct SubSubConfig {

            SubSubConfig() : intVal(), moved(false) {}
            SubSubConfig(const SubSubConfig& config) : intVal(config.intVal), moved(false) {}
            SubSubConfig(SubSubConfig&& config) : intVal(std::move(config.intVal)), moved(false) {
                config.moved = true;
            }
            SubSubConfig& operator=(const SubSubConfig& config) {
                intVal = config.intVal;
                moved = false;
                return *this;
            }
            SubSubConfig& operator=(SubSubConfig&& config) {
                intVal = std::move(config.intVal);
                moved = false;
                config.moved = true;
                return *this;
            }
            bool isMoved() const {
                return moved;
            }

            int intVal;
            bool moved;
            CARGO_REGISTER
            (
                intVal
            )
        };

        int intVal;
        std::vector<int> intVector;
        SubSubConfig subSubObj;

        CARGO_REGISTER
        (
            intVal,
            intVector,
            subSubObj
        )
    };

    struct SubConfigOption {
        CARGO_DECLARE_UNION
        (
            SubConfig,
            int
        )
    };

    typedef std::tuple<std::string, std::pair<int, double>> ComplexTuple;

    std::int8_t int8Val;
    std::int16_t int16Val;
    int intVal;
    std::int64_t int64Val;
    std::uint8_t uint8Val;
    std::uint32_t uint32Val;
    std::uint64_t uint64Val;
    std::string stringVal;
    double doubleVal;
    bool boolVal;
    TestEnum enumVal;

    std::vector<int> emptyIntVector;
    std::vector<int> intVector;
    std::vector<std::string> stringVector;
    std::vector<double> doubleVector;

    std::array<int, 2> intArray;

    std::pair<int, int> intIntPair;
    ComplexTuple complexTuple;
    std::tuple<SubConfig> subObjTuple;
    std::pair<SubConfig, int> subObjIntPair;

    SubConfig subObj;
    std::vector<SubConfig> subVector;

    SubConfigOption union1;
    SubConfigOption union2;
    std::vector<SubConfigOption> unions;
    std::map<std::string, std::string> simpleMap;
    std::map<std::string, TestMapStruct> map;
    std::string dirPath;
    std::string filePath;

    static bool isShorter(const std::string &shorter, const std::string &longer) {
        return shorter.size() < longer.size();
    }

    CARGO_REGISTER
    (
        int8Val,
        int16Val,
        intVal,
        int64Val,
        uint8Val,
        uint32Val,
        uint64Val,
        stringVal,
        doubleVal,
        boolVal,
        enumVal,

        emptyIntVector,
        intVector,
        stringVector,
        doubleVector,

        intArray,

        intIntPair,
        complexTuple,
        subObjTuple,
        subObjIntPair,

        subObj,
        subVector,

        union1,
        union2,
        unions,

        simpleMap,
        map,
        dirPath,
        filePath
    )

    CARGO_VALIDATE
    (
        using namespace std::placeholders;

        CARGO_CHECK(cargo::validator::isNonEmptyString, stringVal, dirPath)
        CARGO_CHECK(std::bind(cargo::validator::isNonEmptyString, _1), filePath);
        CARGO_CHECK(cargo::validator::isAbsolutePath, dirPath, filePath)
        CARGO_CHECK(cargo::validator::isFilePresent, filePath)
        CARGO_CHECK(cargo::validator::isDirectoryPresent, dirPath)

        // custom validator
        CARGO_CHECK([](const std::string & v)->bool { return v.compare("blah")==0; }, stringVal)

        CARGO_COMPARE([](const std::uint32_t arg_A,
                         const std::uint64_t arg_B) {
                             return arg_A < arg_B;
                         },
                         int8Val,
                         uint8Val)
        CARGO_COMPARE(std::bind(isShorter, _1, _2), filePath, dirPath)
    )
};

struct PartialTestConfig {
    // subtree class
    struct SubConfig {
        // a subset of TestConfig::SubConfig fields
        int intVal = 64;

        CARGO_REGISTER
        (
            intVal
        )
    };

    struct SubConfigOption {
        CARGO_DECLARE_UNION
        (
            SubConfig,
            int
        )
    };

    // a subset of TestConfig fields
    std::string stringVal = "partialConfig";
    std::vector<int> intVector = {1, 2, 4, 8, 16};
    TestEnum enumVal = TestEnum::THIRD;
    std::vector<SubConfig> subVector = {SubConfig()};
    SubConfigOption union1;

    PartialTestConfig() {
        union1.set(SubConfig());
    }

    CARGO_REGISTER
    (
        stringVal,
        intVector,
        enumVal,
        subVector,
        union1
    )
};

struct IncompatibleTestConfig: public PartialTestConfig {
    std::array<int, 3> intArray = {{1, 2 ,4}};

    CARGO_EXTEND(PartialTestConfig)
    (
        intArray
    )
};

/**
 * JSON string used in ConfigSuite test cases
 * For the purpose of these tests the order of this string
 * has to be equal to the above REGISTER order
 */
const std::string jsonTestString =
    "{ \"int8Val\": 5, "
    "\"int16Val\": 11235, "
    "\"intVal\": 12345, "
    "\"int64Val\": -1234567890123456789, "
    "\"uint8Val\": 42, "
    "\"uint32Val\": 123456, "
    "\"uint64Val\": 1234567890123456789, "
    "\"stringVal\": \"blah\", "
    "\"doubleVal\": -1.234000, "
    "\"boolVal\": true, "
    "\"enumVal\": 12, "
    "\"emptyIntVector\": [ ], "
    "\"intVector\": [ 1, 2, 3 ], "
    "\"stringVector\": [ \"a\", \"b\" ], "
    "\"doubleVector\": [ 0.000000, 1.000000, 2.000000 ], "
    "\"intArray\": [ 0, 1 ], "
    "\"intIntPair\": [ 8, 9 ], "
    "\"complexTuple\": [ \"tuple\", [ 54, -1.234000 ] ], "
    "\"subObjTuple\": [ { \"intVal\": 54321, \"intVector\": [ 1, 2 ], \"subSubObj\": { \"intVal\": 234 } } ], "
    "\"subObjIntPair\": [ { \"intVal\": 54321, \"intVector\": [ 1, 2 ], \"subSubObj\": { \"intVal\": 234 } }, 50 ], "
    "\"subObj\": { \"intVal\": 54321, \"intVector\": [ 1, 2 ], \"subSubObj\": { \"intVal\": 234 } }, "
    "\"subVector\": [ { \"intVal\": 123, \"intVector\": [ 3, 4 ], \"subSubObj\": { \"intVal\": 345 } }, "
    "{ \"intVal\": 456, \"intVector\": [ 5, 6 ], \"subSubObj\": { \"intVal\": 567 } } ], "
    "\"union1\": { \"type\": \"int\", \"value\": 2 }, "
    "\"union2\": { \"type\": \"SubConfig\", \"value\": { \"intVal\": 54321, \"intVector\": [ 1 ], "
    "\"subSubObj\": { \"intVal\": 234 } } }, "
    "\"unions\": [ "
    "{ \"type\": \"int\", \"value\": 2 }, "
    "{ \"type\": \"SubConfig\", \"value\": { \"intVal\": 54321, \"intVector\": [ 1 ], "
    "\"subSubObj\": { \"intVal\": 234 } } } ], "
    "\"simpleMap\": { \"key\": \"value\", \"key2\": \"value2\" }, "
    "\"map\": { \"dev\": { \"type\": \"tmpfs\", \"source\": \"tmpfs\", \"options\": "
                               "[ \"nosuid\", \"strictatime\", \"mode=755\", \"size=65536k\" ] }, "
               "\"proc\": { \"type\": \"proc\", \"source\": \"proc\", \"options\": [ ] } }, "
    "\"dirPath\": \"\\/usr\\/local\\/lib\", "
    "\"filePath\": \"\\/bin\\/bash\" }";

const std::string jsonEmptyTestString =
    "{ \"int8Val\": 0, "
    "\"int16Val\": 0, "
    "\"intVal\": 0, "
    "\"int64Val\": 0, "
    "\"uint8Val\": 0, "
    "\"uint32Val\": 0, "
    "\"uint64Val\": 0, "
    "\"stringVal\": \"\", "
    "\"doubleVal\": 0.0, "
    "\"boolVal\": false, "
    "\"enumVal\": 0, "
    "\"emptyIntVector\": [ ], "
    "\"intVector\": [ ], "
    "\"stringVector\": [ ], "
    "\"doubleVector\": [ ], "
    "\"intArray\": [ 0, 0 ], "
    "\"intIntPair\": [ 0, 0 ], "
    "\"complexTuple\": [ \"\", [ 0, 0.0 ] ], "
    "\"subObjTuple\": [ { \"intVal\": 0, \"intVector\": [ 0, 0 ], \"subSubObj\": { \"intVal\": 0 } } ], "
    "\"subObjIntPair\": [ { \"intVal\": 0, \"intVector\": [ 0, 0 ], \"subSubObj\": { \"intVal\": 0 } }, 0 ], "
    "\"subObj\": { \"intVal\": 0, \"intVector\": [ ], \"subSubObj\": { \"intVal\": 0 } }, "
    "\"subVector\": [ ], "
    "\"union1\": { \"type\": \"int\", \"value\": 0 }, "
    "\"union2\": { \"type\": \"int\", \"value\": 0 }, "
    "\"unions\": [ ], "
    "\"simpleMap\": { }, "
    "\"map\": { }, "
    "\"dirPath\": \"\", "
    "\"filePath\": \"\" }";

#endif
