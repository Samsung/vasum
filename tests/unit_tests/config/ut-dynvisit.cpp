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
 * @brief   Unit test of combine kvstore with defaults from json
 */

#include "config.hpp"
#include "ut.hpp"
#include "testconfig-example.hpp"
#include "config/manager.hpp"
#include <boost/filesystem.hpp>

#include "config/from-kvjson-visitor.hpp"


using namespace config;
namespace fs = boost::filesystem;

struct Fixture {
    std::string dbPath;

    Fixture()
        : dbPath(fs::unique_path("/tmp/kvstore-%%%%.db3").string())
    {
        fs::remove(dbPath);
    }
    ~Fixture()
    {
        fs::remove(dbPath);
    }
};

BOOST_FIXTURE_TEST_SUITE(DynVisitSuite, Fixture)

void checkJsonConfig(const TestConfig& cfg, const std::string& json)
{
    TestConfig cfg2;
    loadFromString(json, cfg2);
    BOOST_CHECK_EQUAL(cfg2.intVal, cfg.intVal);
    BOOST_CHECK_EQUAL(cfg.int64Val, cfg.int64Val);
    BOOST_CHECK_EQUAL(cfg2.boolVal, cfg.boolVal);
    BOOST_CHECK_EQUAL(cfg2.stringVal, cfg.stringVal);
    BOOST_CHECK_EQUAL(cfg2.intVector.size(), cfg.intVector.size());
    BOOST_CHECK_EQUAL(cfg2.subObj.intVal, cfg.subObj.intVal);
}

void checkKVConfig(const TestConfig& cfg, const std::string& db)
{
    KVStore store(db);
    BOOST_CHECK_EQUAL(store.get<int>(".intVal"), cfg.intVal);
    BOOST_CHECK_EQUAL(store.get<int64_t>(".int64Val"), cfg.int64Val);
    BOOST_CHECK_EQUAL(store.get<bool>(".boolVal"), cfg.boolVal);
    BOOST_CHECK_EQUAL(store.get<std::string>(".stringVal"), cfg.stringVal);
    BOOST_CHECK_EQUAL(store.get<int>(".intVector"), cfg.intVector.size());
    BOOST_CHECK_EQUAL(store.get<int>(".subObj.intVal"), cfg.subObj.intVal);
}

BOOST_AUTO_TEST_CASE(ReadConfigDefaults)
{
    TestConfig cfg;
    loadFromKVStoreWithJson(dbPath, jsonTestString, cfg);
    checkJsonConfig(cfg, jsonTestString);
}

BOOST_AUTO_TEST_CASE(ReadConfigNoDefaults)
{
    TestConfig cfg;
    loadFromKVStoreWithJson(dbPath, jsonTestString, cfg);
    // modify and save config
    cfg.intVal += 5;
    cfg.int64Val += 7777;
    cfg.boolVal = !cfg.boolVal;
    cfg.stringVal += "-changed";
    config::saveToKVStore(dbPath, cfg);

    TestConfig cfg2;
    loadFromKVStoreWithJson(dbPath, jsonTestString, cfg2);
    checkKVConfig(cfg2, dbPath);
}

BOOST_AUTO_TEST_SUITE_END()