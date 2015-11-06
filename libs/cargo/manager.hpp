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
 * @ingroup libCargo
 * @brief   Cargo management functions
 *
 * Example of various data formats operations:
 *
 * @code
 * #include "cargo/fields.hpp"
 * #include "cargo/manager.hpp"
 * #include <iostream>
 * #include <cstdio>
 *
 * struct Foo
 * {
 *     std::string bar = "plain-text";
 *     std::vector<int> tab = std::vector<int>{1, 2, 4, 8};
 *     double number = 3.14;
 *
 *     CARGO_REGISTER
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
 *     const std::string jsonString = cargo::saveToJsonString(foo);
 *     cargo::loadFromJsonString(jsonString, foo);
 *
 *     const GVariant* gVariantPointer = cargo::saveToGVariant(foo);
 *     cargo::loadFromGVariant(gVariantPointer, foo);
 *     g_variant_unref(gVariantPointer);
 *
 *     constexpr std::string jsonFile = "foo.json";
 *     cargo::saveToJsonFile(jsonFile, foo);
 *     cargo::loadFromJsonFile(jsonFile, foo);
 *
 *     constexpr std::string kvDBPath = "kvdb";
 *     constexpr std::string key = "foo";
 *     cargo::saveToKVStore(kvDBPath, foo, key);
 *     cargo::loadFromKVStore(kvDBPath, foo, key);
 *
 *     cargo::loadFromKVStoreWithJson(kvDBPath, jsonString, foo, key);
 *     cargo::loadFromKVStoreWithJsonFile(kvDBPath, jsonFile, foo, key);
 *
 *     FILE* file = fopen("blob", "wb");
 *     if (!file)
 *     {
 *         return EXIT_FAILURE;
 *     }
 *     const int fd = ::fileno(file);
 *     cargo::saveToFD(fd, foo);
 *     ::fclose(file);
 *     file = ::fopen("blob", "rb");
 *     if(!file) {
 *         return EXIT_FAILURE;
 *     }
 *     cargo::loadFromFD(fd, foo);
 *     ::fclose(file);
 *
 *     return 0;
 * }
 * @endcode
 */

#ifndef COMMON_CARGO_MANAGER_HPP
#define COMMON_CARGO_MANAGER_HPP

#include "cargo/to-gvariant-visitor.hpp"
#include "cargo/to-json-visitor.hpp"
#include "cargo/to-kvstore-visitor.hpp"
#include "cargo/to-fdstore-visitor.hpp"
#include "cargo/from-gvariant-visitor.hpp"
#include "cargo/from-json-visitor.hpp"
#include "cargo/from-kvstore-visitor.hpp"
#include "cargo/from-kvstore-ignoring-visitor.hpp"
#include "cargo/from-fdstore-visitor.hpp"
#include "cargo/fs-utils.hpp"
#include "cargo/is-union.hpp"
#include "logger/logger.hpp"

namespace cargo {

/*@{*/

/**
 * Fills the Cargo structure with data stored in the GVariant
 *
 * @param gvariant      data in GVariant type
 * @param visitable     visitable structure to fill
 */
template <class Cargo>
void loadFromGVariant(GVariant* gvariant, Cargo& visitable)
{
    static_assert(isVisitable<Cargo>::value, "Use CARGO_REGISTER macro");
    static_assert(!isUnion<Cargo>::value, "Don't use CARGO_DECLARE_UNION in top level visitable");

    FromGVariantVisitor visitor(gvariant);
    visitable.accept(visitor);
}

/**
 * Saves the visitable in a GVariant
 *
 * @param visitable   visitable structure to convert
 */
template <class Cargo>
GVariant* saveToGVariant(const Cargo& visitable)
{
    static_assert(isVisitable<Cargo>::value, "Use CARGO_REGISTER macro");
    static_assert(!isUnion<Cargo>::value, "Don't use CARGO_DECLARE_UNION in top level visitable");

    ToGVariantVisitor visitor;
    visitable.accept(visitor);
    return visitor.toVariant();
}

/**
 * Fills the visitable with data stored in the json string
 *
 * @param jsonString    data in a json format
 * @param visitable        visitable structure to fill
 */
template <class Cargo>
void loadFromJsonString(const std::string& jsonString, Cargo& visitable)
{
    static_assert(isVisitable<Cargo>::value, "Use CARGO_REGISTER macro");

    FromJsonVisitor visitor(jsonString);
    visitable.accept(visitor);
}

/**
 * Creates a string representation of the visitable in json format
 *
 * @param visitable   visitable structure to convert
 */
template <class Cargo>
std::string saveToJsonString(const Cargo& visitable)
{
    static_assert(isVisitable<Cargo>::value, "Use CARGO_REGISTER macro");

    ToJsonVisitor visitor;
    visitable.accept(visitor);
    return visitor.toString();
}

/**
 * Loads the visitable from a json file
 *
 * @param filename    path to the file
 * @param visitable   visitable structure to load
 */
template <class Cargo>
void loadFromJsonFile(const std::string& filename, Cargo& visitable)
{
    std::string content;
    if (!fsutils::readFileContent(filename, content)) {
        const std::string& msg = "Could not load " + filename;
        LOGE(msg);
        throw CargoException(msg);
    }
    try {
        loadFromJsonString(content, visitable);
    } catch (CargoException& e) {
        const std::string& msg = "Error in " + filename + ": " + e.what();
        LOGE(msg);
        throw CargoException(msg);
    }
}

/**
 * Saves the visitable in a json file
 *
 * @param filename    path to the file
 * @param visitable   visitable structure to save
 */
template <class Cargo>
void saveToJsonFile(const std::string& filename, const Cargo& visitable)
{
    const std::string content = saveToJsonString(visitable);
    if (!fsutils::saveFileContent(filename, content)) {
        throw CargoException("Could not save " + filename);
    }
}

/**
 * Loads a visitable structure from KVStore.
 *
 * @param filename      path to the KVStore db
 * @param visitable     visitable structure to load
 * @param visitableName name of the structure inside the KVStore db
 */
template <class Cargo>
void loadFromKVStore(const std::string& filename, Cargo& visitable, const std::string& visitableName)
{
    static_assert(isVisitable<Cargo>::value, "Use CARGO_REGISTER macro");

    KVStore store(filename);
    KVStore::Transaction transaction(store);
    FromKVStoreVisitor visitor(store, visitableName);
    visitable.accept(visitor);
    transaction.commit();
}

/**
 * Saves the visitable to a KVStore.
 *
 * @param filename      path to the KVStore db
 * @param visitable     visitable structure to save
 * @param visitableName name of the structure inside the KVStore db
 */
template <class Cargo>
void saveToKVStore(const std::string& filename, const Cargo& visitable, const std::string& visitableName)
{
    static_assert(isVisitable<Cargo>::value, "Use CARGO_REGISTER macro");

    KVStore store(filename);
    KVStore::Transaction transaction(store);
    ToKVStoreVisitor visitor(store, visitableName);
    visitable.accept(visitor);
    transaction.commit();
}

/**
 * Load the visitable from KVStore with defaults given in json
 *
 * @param kvfile            path to the KVStore db
 * @param json              path to json file with defaults
 * @param visitable         visitable structure to save
 * @param kvVisitableName   name of the structure inside the KVStore db
 */
template <class Cargo>
void loadFromKVStoreWithJson(const std::string& kvfile,
                             const std::string& json,
                             Cargo& visitable,
                             const std::string& kvVisitableName)
{
    static_assert(isVisitable<Cargo>::value, "Use CARGO_REGISTER macro");

    KVStore store(kvfile);
    KVStore::Transaction transaction(store);
    FromJsonVisitor fromJsonVisitor(json);
    FromKVStoreIgnoringVisitor fromKVStoreVisitor(store, kvVisitableName);
    visitable.accept(fromJsonVisitor);
    visitable.accept(fromKVStoreVisitor);
    transaction.commit();
}

/**
 * Load the data from KVStore with defaults given in json file
 *
 * @param kvfile            path to the KVStore db
 * @param jsonfile          path to json file with defaults
 * @param visitable         visitable structure to save
 * @param kvVisitableName   name of the structure inside the KVStore db
 */
template <class Cargo>
void loadFromKVStoreWithJsonFile(const std::string& kvfile,
                                 const std::string& jsonfile,
                                 Cargo& visitable,
                                 const std::string& kvVisitableName)
{
    std::string content;
    if (!fsutils::readFileContent(jsonfile, content)) {
        throw CargoException("Could not load " + jsonfile);
    }
    try {
        loadFromKVStoreWithJson(kvfile, content, visitable, kvVisitableName);
    } catch (CargoException& e) {
        throw CargoException("Error in " + jsonfile + ": " + e.what());
    }
}

/**
 * Load binary data from a file/socket/pipe represented by the fd
 *
 * @param fd        file descriptor
 * @param visitable visitable structure to load
 */
template <class Cargo>
void loadFromFD(const int fd, Cargo& visitable)
{
    static_assert(isVisitable<Cargo>::value, "Use CARGO_REGISTER macro");

    FromFDStoreVisitor visitor(fd);
    visitable.accept(visitor);
}

/**
 * Save binary data to a file/socket/pipe represented by the fd
 *
 * @param fd        file descriptor
 * @param visitable visitable structure to save
 */
template <class Cargo>
void saveToFD(const int fd, const Cargo& visitable)
{
    static_assert(isVisitable<Cargo>::value, "Use CARGO_REGISTER macro");

    ToFDStoreVisitor visitor(fd);
    visitable.accept(visitor);
}

} // namespace cargo

/*@}*/

#endif // COMMON_CARGO_MANAGER_HPP
