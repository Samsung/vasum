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
 * @author  Pawel Kubik (p.kubik@samsung.com)
 * @brief   Default visitor for loading from KVStore
 */

#ifndef CARGO_SQLITE_INTERNALS_FROM_KVSTORE_VISITOR_HPP
#define CARGO_SQLITE_INTERNALS_FROM_KVSTORE_VISITOR_HPP

#include "cargo-sqlite/internals/from-kvstore-visitor-base.hpp"


namespace cargo {

namespace internals {

/**
 * Default KVStore visitor.
 */
class FromKVStoreVisitor : public FromKVStoreVisitorBase<FromKVStoreVisitor> {
public:
    FromKVStoreVisitor(KVStore& store, const std::string& prefix)
        : FromKVStoreVisitorBase<FromKVStoreVisitor>(store, prefix)
    {
    }

    FromKVStoreVisitor(const FromKVStoreVisitorBase<FromKVStoreVisitor>& visitor,
                       const std::string& prefix)
        : FromKVStoreVisitorBase<FromKVStoreVisitor>(visitor, prefix)
    {
    }
};

} // namespace internals

} // namespace cargo

#endif // CARGO_SQLITE_INTERNALS_FROM_KVSTORE_VISITOR_HPP
