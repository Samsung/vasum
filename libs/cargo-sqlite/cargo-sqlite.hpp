/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
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
 * @defgroup libcargo-sqlite libcargo-sqlite
 * @brief    cargo KVStore interface
 */

#ifndef CARGO_SQLITE_CARGO_SQLITE_HPP
#define CARGO_SQLITE_CARGO_SQLITE_HPP

#include "cargo-sqlite/internals/to-kvstore-visitor.hpp"
#include "cargo-sqlite/internals/from-kvstore-visitor.hpp"
#include "cargo-sqlite/internals/from-kvstore-ignoring-visitor.hpp"

namespace cargo {

/*@{*/

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
    static_assert(internals::isVisitable<Cargo>::value, "Use CARGO_REGISTER macro");

    internals::KVStore store(filename);
    internals::KVStore::Transaction transaction(store);
    internals::FromKVStoreVisitor visitor(store, visitableName);
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
    static_assert(internals::isVisitable<Cargo>::value, "Use CARGO_REGISTER macro");

    internals::KVStore store(filename);
    internals::KVStore::Transaction transaction(store);
    internals::ToKVStoreVisitor visitor(store, visitableName);
    visitable.accept(visitor);
    transaction.commit();
}

} // namespace cargo

/*@}*/

#endif // CARGO_SQLITE_CARGO_SQLITE_HPP
