/*
 *  Copyright (c) 2000 - 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Bumjin Im <bj.im@samsung.com>
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
 * @file    ut-scs-configuration.cpp
 * @author  Michal Witanowski (m.witanowski@samsung.com)
 * @brief   Unit test of ConfigurationBase
 */

#include "scs-configuration.hpp"

#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

using namespace security_containers;

BOOST_AUTO_TEST_SUITE(ConfigSuite)

struct TestConfig : public ConfigurationBase
{
    // subtree class
    struct SubConfig : public ConfigurationBase
    {
        int intVal;

        CONFIG_REGISTER
        {
            CONFIG_VALUE(intVal)
        }

        bool operator== (const SubConfig& rhs) const
        {
            return (rhs.intVal == intVal);
        }
    };

    int intVal;
    std::string stringVal;
    double floatVal;
    bool boolVal;

    std::vector<int> intVector;
    std::vector<std::string> stringVector;
    std::vector<double> floatVector;

    SubConfig subObj;
    std::vector<SubConfig> subVector;

    CONFIG_REGISTER
    {
        CONFIG_VALUE(intVal)
        CONFIG_VALUE(stringVal)
        CONFIG_VALUE(floatVal)
        CONFIG_VALUE(boolVal)

        CONFIG_VALUE(intVector)
        CONFIG_VALUE(stringVector)
        CONFIG_VALUE(floatVector)

        CONFIG_SUB_OBJECT(subObj)
        CONFIG_SUB_OBJECT(subVector)
    }

    bool operator== (const TestConfig& rhs) const
    {
        return (rhs.intVal == intVal &&
                rhs.stringVal == stringVal &&
                rhs.floatVal == floatVal &&
                rhs.boolVal == boolVal &&
                rhs.subObj == subObj &&
                rhs.intVector == intVector &&
                rhs.stringVector == stringVector &&
                rhs.floatVector == floatVector &&
                rhs.subVector == subVector);
    }
};

/**
 * JSON string used in ConfigSuite test cases
 */
const std::string json_test_string =
    "{ \"intVal\": 12345, "
    "\"stringVal\": \"blah\", "
    "\"floatVal\": -1.234, "
    "\"boolVal\": true, "
    "\"intVector\": [1, 2, 3], "
    "\"stringVector\": [\"a\", \"b\"], "
    "\"floatVector\": [0.0, 1.0, 2.0], "
    "\"subObj\": { \"intVal\": 54321 }, "
    "\"subVector\": [ { \"intVal\": 123 }, { \"intVal\": 456 } ] }";

const double max_float_error = 1.0e-10;


BOOST_AUTO_TEST_CASE(SimpleTypesTest)
{
    TestConfig testConfig;

    // make sure that parseStr() does not throw an exception
    BOOST_REQUIRE_NO_THROW(testConfig.parseStr(json_test_string));

    BOOST_CHECK_EQUAL(12345, testConfig.intVal);
    BOOST_CHECK_CLOSE(-1.234, testConfig.floatVal, max_float_error);
    BOOST_CHECK_EQUAL("blah", testConfig.stringVal);
    BOOST_CHECK_EQUAL(true, testConfig.boolVal);
}

BOOST_AUTO_TEST_CASE(IntVectorTest)
{
    TestConfig testConfig;
    BOOST_REQUIRE_NO_THROW(testConfig.parseStr(json_test_string));

    BOOST_REQUIRE_EQUAL(3, testConfig.intVector.size());
    BOOST_CHECK_EQUAL(1, testConfig.intVector[0]);
    BOOST_CHECK_EQUAL(2, testConfig.intVector[1]);
    BOOST_CHECK_EQUAL(3, testConfig.intVector[2]);
}

BOOST_AUTO_TEST_CASE(StringVectorTest)
{
    TestConfig testConfig;
    BOOST_REQUIRE_NO_THROW(testConfig.parseStr(json_test_string));

    BOOST_REQUIRE_EQUAL(2, testConfig.stringVector.size());
    BOOST_CHECK_EQUAL("a", testConfig.stringVector[0]);
    BOOST_CHECK_EQUAL("b", testConfig.stringVector[1]);
}

BOOST_AUTO_TEST_CASE(FloatVectorTest)
{
    TestConfig testConfig;
    BOOST_REQUIRE_NO_THROW(testConfig.parseStr(json_test_string));

    BOOST_REQUIRE_EQUAL(3, testConfig.floatVector.size());
    BOOST_CHECK_CLOSE(0.0, testConfig.floatVector[0], max_float_error);
    BOOST_CHECK_CLOSE(1.0, testConfig.floatVector[1], max_float_error);
    BOOST_CHECK_CLOSE(2.0, testConfig.floatVector[2], max_float_error);
}

BOOST_AUTO_TEST_CASE(SubObjectTest)
{
    TestConfig testConfig;
    BOOST_REQUIRE_NO_THROW(testConfig.parseStr(json_test_string));

    BOOST_CHECK_EQUAL(54321, testConfig.subObj.intVal);
    // TODO: more attributes in subtree
}

BOOST_AUTO_TEST_CASE(SubObjectVectorTest)
{
    TestConfig testConfig;
    BOOST_REQUIRE_NO_THROW(testConfig.parseStr(json_test_string));

    BOOST_REQUIRE_EQUAL(2, testConfig.subVector.size());
    BOOST_CHECK_EQUAL(123, testConfig.subVector[0].intVal);
    BOOST_CHECK_EQUAL(456, testConfig.subVector[1].intVal);
}

BOOST_AUTO_TEST_CASE(ToStringTest)
{
    TestConfig testConfig;
    BOOST_REQUIRE_NO_THROW(testConfig.parseStr(json_test_string));

    std::cout << "Input:" << std::endl << json_test_string << std::endl;

    std::string out;
    BOOST_REQUIRE_NO_THROW(out = testConfig.toString());
    std::cout << std::endl << "Output:" << std::endl << out << std::endl;

    // parse output again and check if both objects match
    std::cout << "Validating output..." << std::endl;
    TestConfig outputConfig;
    BOOST_REQUIRE_NO_THROW(outputConfig.parseStr(json_test_string));
    BOOST_CHECK(outputConfig == testConfig);
}

BOOST_AUTO_TEST_SUITE_END()
