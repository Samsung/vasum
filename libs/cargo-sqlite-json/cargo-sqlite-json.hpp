/*
 *  Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Pawel Kubik (p.kubik@samsung.com)
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
 * @author   Pawel Kubik (p.kubik@samsung.com)
 * @defgroup libcargo-sqlite-json libcargo-sqlite-json
 * @brief    cargo SQLite with Json interface
 */

#ifndef CARGO_SQLITE_JSON_CARGO_SQLITE_JSON_HPP
#define CARGO_SQLITE_JSON_CARGO_SQLITE_JSON_HPP

#include "cargo-json/to-json-visitor.hpp"
#include "cargo-sqlite/to-kvstore-visitor.hpp"
#include "cargo-json/from-json-visitor.hpp"
#include "cargo-sqlite/from-kvstore-visitor.hpp"
#include "cargo-sqlite/from-kvstore-ignoring-visitor.hpp"
#include "cargo-json/fs-utils.hpp"

namespace cargo {

/*@{*/

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

} // namespace cargo

/*@}*/

#endif // CARGO_SQLITE_JSON_CARGO_SQLITE_JSON_HPP