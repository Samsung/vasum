/*
 *  Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Maciej Karpiuk <m.karpiuk2@samsung.com>
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
 * @author  Maciej Karpiuk <m.karpiuk2@samsung.com>
 * @brief   Unit test for cargo field validation
 */

#include "config.hpp"
#include "ut.hpp"
#include "cargo-json/cargo-json.hpp"
#include "testconfig-example.hpp"
#include "cargo-validator/exception.hpp"
#include "cargo-validator/validator.hpp"

using namespace cargo;
using namespace validator;

BOOST_AUTO_TEST_SUITE(CargoValidatorSuite)

BOOST_AUTO_TEST_CASE(SuccessfulRun)
{
    TestConfig testConfig;
    BOOST_REQUIRE_NO_THROW(loadFromJsonString(jsonTestString, testConfig));

    BOOST_REQUIRE_NO_THROW(validate(testConfig));
}

BOOST_AUTO_TEST_CASE(EmptyContents)
{
    TestConfig testConfig;
    BOOST_REQUIRE_NO_THROW(loadFromJsonString(jsonEmptyTestString, testConfig));

    BOOST_REQUIRE_THROW(validate(testConfig), VerificationException);
}

BOOST_AUTO_TEST_CASE(OneFieldModified)
{
    TestConfig testConfig;
    BOOST_REQUIRE_NO_THROW(loadFromJsonString(jsonTestString, testConfig));
    testConfig.stringVal = std::string("wrong");

    BOOST_REQUIRE_THROW(validate(testConfig), VerificationException);
}

BOOST_AUTO_TEST_CASE(TwoFieldRelationship)
{
    TestConfig testConfig;
    BOOST_REQUIRE_NO_THROW(loadFromJsonString(jsonTestString, testConfig));
    testConfig.int8Val = 127;

    BOOST_REQUIRE_THROW(validate(testConfig), VerificationException);
}

BOOST_AUTO_TEST_CASE(FileNotPresent)
{
    TestConfig testConfig;
    BOOST_REQUIRE_NO_THROW(loadFromJsonString(jsonTestString, testConfig));
    testConfig.filePath = std::string("coco jumbo");

    BOOST_REQUIRE_THROW(validator::validate(testConfig), VerificationException);
}

BOOST_AUTO_TEST_CASE(FilePointsToDirectory)
{
    TestConfig testConfig;
    BOOST_REQUIRE_NO_THROW(loadFromJsonString(jsonTestString, testConfig));
    testConfig.filePath = std::string("/usr");

    BOOST_REQUIRE_THROW(validator::validate(testConfig), VerificationException);
}

BOOST_AUTO_TEST_CASE(DirectoryNotPresent)
{
    TestConfig testConfig;
    BOOST_REQUIRE_NO_THROW(loadFromJsonString(jsonTestString, testConfig));
    testConfig.dirPath = std::string("/cocojumbo");

    BOOST_REQUIRE_THROW(validator::validate(testConfig), VerificationException);
}

BOOST_AUTO_TEST_CASE(NotADirectory)
{
    TestConfig testConfig;
    BOOST_REQUIRE_NO_THROW(loadFromJsonString(jsonTestString, testConfig));
    testConfig.dirPath = std::string("/bin/bash");

    BOOST_REQUIRE_THROW(validator::validate(testConfig), VerificationException);
}

BOOST_AUTO_TEST_CASE(RelativePath)
{
    TestConfig testConfig;
    BOOST_REQUIRE_NO_THROW(loadFromJsonString(jsonTestString, testConfig));
    testConfig.filePath = std::string("../myFile");

    BOOST_REQUIRE_THROW(validator::validate(testConfig), VerificationException);
}

BOOST_AUTO_TEST_SUITE_END()
