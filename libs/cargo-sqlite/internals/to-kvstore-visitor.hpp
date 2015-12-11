/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Jan Olszak (j.olszak@samsung.com)
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
 * @author  Jan Olszak (j.olszak@samsung.com)
 * @brief   Visitor for saving to KVStore
 */

#ifndef CARGO_SQLITE_INTERNALS_TO_KVSTORE_VISITOR_HPP
#define CARGO_SQLITE_INTERNALS_TO_KVSTORE_VISITOR_HPP

#include "cargo-sqlite/internals/kvstore.hpp"
#include "cargo-sqlite/internals/kvstore-visitor-utils.hpp"
#include "cargo/internals/is-visitable.hpp"
#include "cargo/internals/is-like-tuple.hpp"
#include "cargo/internals/is-streamable.hpp"
#include "cargo/internals/visit-fields.hpp"
#include "cargo/exception.hpp"

#include <map>

namespace cargo {

namespace internals {

class ToKVStoreVisitor {

public:
    ToKVStoreVisitor(KVStore& store, const std::string& prefix)
        : mStore(store),
          mKeyPrefix(prefix)
    {
    }

    ToKVStoreVisitor& operator=(const ToKVStoreVisitor&) = delete;

    template<typename T>
    void visit(const std::string& name, const T& value)
    {
        setInternal(key(mKeyPrefix, name), value);
    }

private:
    KVStore& mStore;
    std::string mKeyPrefix;

    ToKVStoreVisitor(const ToKVStoreVisitor& visitor, const std::string& prefix)
        : mStore(visitor.mStore),
          mKeyPrefix(prefix)
    {
    }

    template<typename T, typename std::enable_if<isStreamableOut<T>::value, int>::type = 0>
    void setInternal(const std::string& name, const T& value)
    {
        mStore.set(name, toString(value));
    }

    template<typename T, typename std::enable_if<isVisitable<T>::value, int>::type = 0>
    void setInternal(const std::string& name, const T& value)
    {
        ToKVStoreVisitor visitor(*this, name);
        value.accept(visitor);
    }

    template<typename T, typename std::enable_if<std::is_enum<T>::value, int>::type = 0>
    void setInternal(const std::string& name, const T& value)
    {
        setInternal(name, static_cast<const typename std::underlying_type<T>::type>(value));
    }

    template <typename I>
    void setRangeInternal(const std::string& name,
                          const I& begin,
                          const I& end,
                          const size_t size) {
        KVStore::Transaction transaction(mStore);

        mStore.remove(name);
        setInternal(name, size);
        size_t i = 0;
        for (auto it = begin; it != end; ++it) {
            const std::string k = key(name, std::to_string(i));
            setInternal(k, *it);
            ++i;
        }
        transaction.commit();
    }

    template<typename T>
    void setInternal(const std::string& name, const std::vector<T>& values)
    {
        setRangeInternal(name, values.begin(), values.end(), values.size());
    }

    template<typename T, std::size_t N>
    void setInternal(const std::string& name, const std::array<T, N>& values) {
        setRangeInternal(name, values.begin(), values.end(), N);
    }

    template<typename V>
    void setInternal(const std::string& name, const std::map<std::string, V>& values) {
        KVStore::Transaction transaction(mStore);

        mStore.remove(name);
        setInternal(name, values.size());
        size_t i = 0;
        for (const auto& it : values) {
            const std::string k = key(name, i++);
            setInternal(k, it.first);
            setInternal(k + ".val", it.second);
        }
        transaction.commit();
    }

    template<typename T>
    void setInternal(const std::string& name, const std::initializer_list<T>& values)
    {
        setRangeInternal(name, values.begin(), values.end(), values.size());
    }

    template<typename T, typename std::enable_if<isLikeTuple<T>::value, int>::type = 0>
    void setInternal(const std::string& name, const T& values)
    {
        KVStore::Transaction transaction(mStore);

        setInternal(name, std::tuple_size<T>::value);

        ToKVStoreVisitor recursiveVisitor(*this, name);
        SetTupleVisitor visitor(recursiveVisitor);
        visitFields(values, &visitor);

        transaction.commit();
    }

    struct SetTupleVisitor
    {
    public:
        SetTupleVisitor(ToKVStoreVisitor& visitor) : mVisitor(visitor) {}

        template<typename T>
        void visit(T& value)
        {
            mVisitor.visit(std::to_string(idx), value);
            ++idx;
        }

    private:
        ToKVStoreVisitor& mVisitor;
        size_t idx = 0;
    };
};

} // namespace internals

} // namespace cargo

#endif // CARGO_SQLITE_INTERNALS_TO_KVSTORE_VISITOR_HPP
