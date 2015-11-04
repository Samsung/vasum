/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Piotr Bartosiewicz (p.bartosiewi@partner.samsung.com)
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
 * @author  Piotr Bartosiewicz (p.bartosiewi@partner.samsung.com)
 * @defgroup manager Manager
 * @ingroup libConfig
 * @brief   Configuration management functions
 *
 * Example of various data formats operations:
 *
 * @code
 * #include "config/fields.hpp"
 * #include "config/manager.hpp"
 * #include <iostream>
 * #include <cstdio>
 *
 * struct Foo
 * {
 *     std::string bar = "plain-text";
 *     std::vector<int> tab = std::vector<int>{1, 2, 4, 8};
 *     double number = 3.14;
 *
 *     CONFIG_REGISTER
 *     (
 *         bar,
 *         tab,
 *         number
 *     )
 * };
 *
 * int main()
 * {
 *     Foo foo;
 *
 *     const std::string jsonString = config::saveToJsonString(foo);
 *     config::loadFromJsonString(jsonString, foo);
 *
 *     const GVariant* gVariantPointer = config::saveToGVariant(foo);
 *     config::loadFromGVariant(gVariantPointer, foo);
 *     g_variant_unref(gVariantPointer);
 *
 *     constexpr std::string jsonFile = "foo.json";
 *     config::saveToJsonFile(jsonFile, foo);
 *     config::loadFromJsonFile(jsonFile, foo);
 *
 *     constexpr std::string kvDBPath = "kvdb";
 *     constexpr std::string key = "foo";
 *     config::saveToKVStore(kvDBPath, foo, key);
 *     config::loadFromKVStore(kvDBPath, foo, key);
 *
 *     config::loadFromKVStoreWithJson(kvDBPath, jsonString, foo, key);
 *     config::loadFromKVStoreWithJsonFile(kvDBPath, jsonFile, foo, key);
 *
 *     FILE* file = fopen("blob", "wb");
 *     if (!file)
 *     {
 *         return EXIT_FAILURE;
 *     }
 *     const int fd = ::fileno(file);
 *     config::saveToFD(fd, foo);
 *     ::fclose(file);
 *     file = ::fopen("blob", "rb");
 *     if(!file) {
 *         return EXIT_FAILURE;
 *     }
 *     config::loadFromFD(fd, foo);
 *     ::fclose(file);
 *
 *     return 0;
 * }
 * @endcode
 */

#ifndef COMMON_CONFIG_MANAGER_HPP
#define COMMON_CONFIG_MANAGER_HPP

#include "config/to-gvariant-visitor.hpp"
#include "config/to-json-visitor.hpp"
#include "config/to-kvstore-visitor.hpp"
#include "config/to-fdstore-visitor.hpp"
#include "config/from-gvariant-visitor.hpp"
#include "config/from-json-visitor.hpp"
#include "config/from-kvstore-visitor.hpp"
#include "config/from-kvstore-ignoring-visitor.hpp"
#include "config/from-fdstore-visitor.hpp"
#include "config/fs-utils.hpp"
#include "config/is-union.hpp"
#include "logger/logger.hpp"

namespace config {

/*@{*/

/**
 * Fills the configuration with data stored in the GVariant
 *
 * @param gvariant   configuration in GVariant type
 * @param config     visitable structure to fill
 */
template <class Config>
void loadFromGVariant(GVariant* gvariant, Config& config)
{
    static_assert(isVisitable<Config>::value, "Use CONFIG_REGISTER macro");
    static_assert(!isUnion<Config>::value, "Don't use CONFIG_DECLARE_UNION in top level config");

    FromGVariantVisitor visitor(gvariant);
    config.accept(visitor);
}

/**
 * Saves the config in a GVariant
 *
 * @param config   visitable structure to convert
 */
template <class Config>
GVariant* saveToGVariant(const Config& config)
{
    static_assert(isVisitable<Config>::value, "Use CONFIG_REGISTER macro");
    static_assert(!isUnion<Config>::value, "Don't use CONFIG_DECLARE_UNION in top level config");

    ToGVariantVisitor visitor;
    config.accept(visitor);
    return visitor.toVariant();
}

/**
 * Fills the configuration with data stored in the json string
 *
 * @param jsonString configuration in a json format
 * @param config     visitable structure to fill
 */
template <class Config>
void loadFromJsonString(const std::string& jsonString, Config& config)
{
    static_assert(isVisitable<Config>::value, "Use CONFIG_REGISTER macro");

    FromJsonVisitor visitor(jsonString);
    config.accept(visitor);
}

/**
 * Creates a string representation of the configuration in json format
 *
 * @param config   visitable structure to convert
 */
template <class Config>
std::string saveToJsonString(const Config& config)
{
    static_assert(isVisitable<Config>::value, "Use CONFIG_REGISTER macro");

    ToJsonVisitor visitor;
    config.accept(visitor);
    return visitor.toString();
}

/**
 * Loads the config from a json file
 *
 * @param filename path to the file
 * @param config   visitable structure to load
 */
template <class Config>
void loadFromJsonFile(const std::string& filename, Config& config)
{
    std::string content;
    if (!fsutils::readFileContent(filename, content)) {
        const std::string& msg = "Could not load " + filename;
        LOGE(msg);
        throw ConfigException(msg);
    }
    try {
        loadFromJsonString(content, config);
    } catch (ConfigException& e) {
        const std::string& msg = "Error in " + filename + ": " + e.what();
        LOGE(msg);
        throw ConfigException(msg);
    }
}

/**
 * Saves the config in a json file
 *
 * @param filename path to the file
 * @param config   visitable structure to save
 */
template <class Config>
void saveToJsonFile(const std::string& filename, const Config& config)
{
    const std::string content = saveToJsonString(config);
    if (!fsutils::saveFileContent(filename, content)) {
        throw ConfigException("Could not save " + filename);
    }
}

/**
 * Loads a visitable configuration from KVStore.
 *
 * @param filename   path to the KVStore db
 * @param config     visitable structure to load
 * @param configName name of the configuration inside the KVStore db
 */
template <class Config>
void loadFromKVStore(const std::string& filename, Config& config, const std::string& configName)
{
    static_assert(isVisitable<Config>::value, "Use CONFIG_REGISTER macro");

    KVStore store(filename);
    KVStore::Transaction transaction(store);
    FromKVStoreVisitor visitor(store, configName);
    config.accept(visitor);
    transaction.commit();
}

/**
 * Saves the config to a KVStore.
 *
 * @param filename   path to the KVStore db
 * @param config     visitable structure to save
 * @param configName name of the config inside the KVStore db
 */
template <class Config>
void saveToKVStore(const std::string& filename, const Config& config, const std::string& configName)
{
    static_assert(isVisitable<Config>::value, "Use CONFIG_REGISTER macro");

    KVStore store(filename);
    KVStore::Transaction transaction(store);
    ToKVStoreVisitor visitor(store, configName);
    config.accept(visitor);
    transaction.commit();
}

/**
 * Load the config from KVStore with defaults given in json
 *
 * @param kvfile    path to the KVStore db
 * @param json      path to json file with defaults
 * @param config    visitable structure to save
 * @param kvConfigName name of the config inside the KVStore db
 */
template <class Config>
void loadFromKVStoreWithJson(const std::string& kvfile,
                             const std::string& json,
                             Config& config,
                             const std::string& kvConfigName)
{
    static_assert(isVisitable<Config>::value, "Use CONFIG_REGISTER macro");

    KVStore store(kvfile);
    KVStore::Transaction transaction(store);
    FromJsonVisitor fromJsonVisitor(json);
    FromKVStoreIgnoringVisitor fromKVStoreVisitor(store, kvConfigName);
    config.accept(fromJsonVisitor);
    config.accept(fromKVStoreVisitor);
    transaction.commit();
}

/**
 * Load the config from KVStore with defaults given in json file
 *
 * @param kvfile    path to the KVStore db
 * @param jsonfile  path to json file with defaults
 * @param config    visitable structure to save
 * @param kvConfigName name of the config inside the KVStore db
 */
template <class Config>
void loadFromKVStoreWithJsonFile(const std::string& kvfile,
                                 const std::string& jsonfile,
                                 Config& config,
                                 const std::string& kvConfigName)
{
    std::string content;
    if (!fsutils::readFileContent(jsonfile, content)) {
        throw ConfigException("Could not load " + jsonfile);
    }
    try {
        loadFromKVStoreWithJson(kvfile, content, config, kvConfigName);
    } catch (ConfigException& e) {
        throw ConfigException("Error in " + jsonfile + ": " + e.what());
    }
}

/**
 * Load binary data from a file/socket/pipe represented by the fd
 *
 * @param fd file descriptor
 * @param config visitable structure to load
 */
template <class Config>
void loadFromFD(const int fd, Config& config)
{
    static_assert(isVisitable<Config>::value, "Use CONFIG_REGISTER macro");

    FromFDStoreVisitor visitor(fd);
    config.accept(visitor);
}

/**
 * Save binary data to a file/socket/pipe represented by the fd
 *
 * @param fd file descriptor
 * @param config visitable structure to save
 */
template <class Config>
void saveToFD(const int fd, const Config& config)
{
    static_assert(isVisitable<Config>::value, "Use CONFIG_REGISTER macro");

    ToFDStoreVisitor visitor(fd);
    config.accept(visitor);
}

} // namespace config

/*@}*/

#endif // COMMON_CONFIG_MANAGER_HPP
