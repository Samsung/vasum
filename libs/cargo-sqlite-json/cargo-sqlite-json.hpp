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

#include "cargo-json/internals/to-json-visitor.hpp"
#include "cargo-sqlite/internals/to-kvstore-visitor.hpp"
#include "cargo-json/internals/from-json-visitor.hpp"
#include "cargo-sqlite/internals/from-kvstore-visitor.hpp"
#include "cargo-sqlite/internals/from-kvstore-ignoring-visitor.hpp"
#include "cargo-json/internals/fs-utils.hpp"
// TODO: remove dependencies to other libraries internals

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
    static_assert(internals::isVisitable<Cargo>::value, "Use CARGO_REGISTER macro");

    internals::KVStore store(kvfile);
    internals::KVStore::Transaction transaction(store);
    internals::FromJsonVisitor fromJsonVisitor(json);
    internals::FromKVStoreIgnoringVisitor fromKVStoreVisitor(store, kvVisitableName);
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
    if (!internals::fsutils::readFileContent(jsonfile, content)) {
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
