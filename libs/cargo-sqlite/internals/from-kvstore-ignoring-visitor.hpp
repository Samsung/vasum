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
 * @brief   Visitor for loading from KVStore that doesn't fail on missing values
 */

#ifndef CARGO_SQLITE_INTERNALS_FROM_KVSTORE_IGNORING_VISITOR_HPP
#define CARGO_SQLITE_INTERNALS_FROM_KVSTORE_IGNORING_VISITOR_HPP

#include "cargo-sqlite/internals/from-kvstore-visitor-base.hpp"


namespace cargo {

namespace internals {

/**
 * A variant of KVStoreVisitor that ignore non-existing fields.
 *
 * Allows to partially update visited structures with fields that exist in the KVStore.
 */
class FromKVStoreIgnoringVisitor : public FromKVStoreVisitorBase<FromKVStoreIgnoringVisitor> {
public:
    FromKVStoreIgnoringVisitor(KVStore& store, const std::string& prefix)
        : FromKVStoreVisitorBase<FromKVStoreIgnoringVisitor>(store, prefix)
    {
    }

    FromKVStoreIgnoringVisitor(const FromKVStoreVisitorBase<FromKVStoreIgnoringVisitor>& visitor,
                               const std::string& prefix)
        : FromKVStoreVisitorBase<FromKVStoreIgnoringVisitor>(visitor, prefix)
    {
    }

    FromKVStoreIgnoringVisitor& operator=(const FromKVStoreIgnoringVisitor&) = delete;

protected:
    template<typename T>
    void visitImpl(const std::string& name, T& value)
    {
        try {
            getInternal(name, value);
        } catch (const NoKeyException& e) {
        } catch (const ContainerSizeException& e) {
        }
    }

private:
    FromKVStoreIgnoringVisitor(const FromKVStoreIgnoringVisitor& visitor,
                               const std::string& prefix)
        : FromKVStoreVisitorBase<FromKVStoreIgnoringVisitor>(visitor.mStore, prefix)
    {
    }

    template<typename T,
             typename std::enable_if<isUnion<T>::value, int>::type = 0>
    void getInternal(const std::string& name, T& value)
    {
        std::string type;
        getInternal(key(name, "type"), type);
        if (type.empty()) {
            return;
        }

        FromKVStoreIgnoringVisitor visitor(*this, name);
        value.accept(visitor);
    }

    template<typename T,
             typename std::enable_if<!isUnion<T>::value, int>::type = 0>
    void getInternal(const std::string& name, T& value)
    {
        FromKVStoreVisitorBase<FromKVStoreIgnoringVisitor>::visitImpl(name, value);
    }

    friend class FromKVStoreVisitorBase;
};

} // namespace internals

} // namespace cargo

#endif // CARGO_SQLITE_INTERNALS_FROM_KVSTORE_IGNORING_VISITOR_HPP
