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
 * @brief   Base of visitors for loading from KVStore
 */

#ifndef CARGO_SQLITE_INTERNALS_FROM_KVSTORE_VISITOR_BASE_HPP
#define CARGO_SQLITE_INTERNALS_FROM_KVSTORE_VISITOR_BASE_HPP

#include "cargo-sqlite/internals/kvstore.hpp"
#include "cargo-sqlite/internals/kvstore-visitor-utils.hpp"
#include "cargo/exception.hpp"
#include "cargo/internals/is-visitable.hpp"
#include "cargo/internals/is-like-tuple.hpp"
#include "cargo/internals/is-streamable.hpp"
#include "cargo/internals/is-union.hpp"
#include "cargo/internals/visit-fields.hpp"
#include <map>


namespace cargo {

namespace internals {

/**
 * Base class for KVStore visitors.
 *
 * A curiously recurring template pattern example. This
 * base-class provides a set of recursively called function templates. A child class can
 * modify the base class behaviour in certain cases by defining:
 *  - a set of functions that match those cases
 *  - a template function that match any other case and calls the base class function
 */
template<typename RecursiveVisitor>
class FromKVStoreVisitorBase {
public:
    FromKVStoreVisitorBase& operator=(const FromKVStoreVisitorBase&) = delete;

    template<typename T>
    void visit(const std::string& name, T& value)
    {
        static_cast<RecursiveVisitor*>(this)->visitImpl(key(mKeyPrefix, name), value);
    }

protected:
    KVStore& mStore;
    std::string mKeyPrefix;

    template<typename T>
    void visitImpl(const std::string& name, T& value)
    {
        getInternal(name, value);
    }

    FromKVStoreVisitorBase(KVStore& store, const std::string& prefix)
        : mStore(store),
          mKeyPrefix(prefix)
    {
    }

    FromKVStoreVisitorBase(const FromKVStoreVisitorBase& visitor,
                           const std::string& prefix)
        : mStore(visitor.mStore),
          mKeyPrefix(prefix)
    {
    }

private:
    template<typename T, typename std::enable_if<isStreamableIn<T>::value, int>::type = 0>
    void getInternal(const std::string& name, T& value)
    {
        value = fromString<T>(mStore.get(name));
    }

    template<typename T, typename std::enable_if<isVisitable<T>::value, int>::type = 0>
    void getInternal(const std::string& name, T& value)
    {
        RecursiveVisitor visitor(*this, name);
        value.accept(visitor);
    }

    template<typename T,
             typename std::enable_if<std::is_enum<T>::value, int>::type = 0>
    void getInternal(const std::string& name, T& value)
    {
        auto rawValue = static_cast<typename std::underlying_type<T>::type>(value);
        static_cast<RecursiveVisitor*>(this)->visitImpl(name, rawValue);
        value = static_cast<T>(rawValue);
    }

    template<typename T>
    void getInternal(const std::string& name, std::vector<T>& values)
    {
        size_t storedSize = 0;
        getInternal(name, storedSize);

        if (storedSize == 0) {
            return;
        }

        values.resize(storedSize);
        for (size_t i = 0; i < storedSize; ++i) {
            const std::string k = key(name, std::to_string(i));
            if (!mStore.prefixExists(k)) {
                throw InternalIntegrityException("Corrupted list serialization.");
            }
            static_cast<RecursiveVisitor*>(this)->visitImpl(k, values[i]);
        }
    }

    template<typename T, size_t N>
    void getInternal(const std::string& name, std::array<T, N>& values)
    {
        size_t storedSize = 0;
        getInternal(name, storedSize);

        if (storedSize != values.size()) {
            throw ContainerSizeException("Size of stored array doesn't match provided one.");
        }

        for (size_t i = 0; i < storedSize; ++i) {
            const std::string k = key(name, std::to_string(i));
            if (!mStore.prefixExists(k)) {
                throw InternalIntegrityException("Corrupted list serialization.");
            }
            static_cast<RecursiveVisitor*>(this)->visitImpl(k, values[i]);
        }
    }

    template<typename V>
    void getInternal(const std::string& name, std::map<std::string, V>& values)
    {
        size_t storedSize = 0;
        getInternal(name, storedSize);

        for (size_t i = 0; i < storedSize; ++i) {
            std::string mapKey, k = key(name, i);
            if (!mStore.prefixExists(k)) {
                throw InternalIntegrityException("Corrupted map serialization.");
            }
            static_cast<RecursiveVisitor*>(this)->visitImpl(k, mapKey);
            static_cast<RecursiveVisitor*>(this)->visitImpl(k + ".val", values[mapKey]);
        }
    }

    template<typename T, typename std::enable_if<isLikeTuple<T>::value, int>::type = 0>
    void getInternal(const std::string& name, T& values)
    {
        size_t storedSize = 0;
        getInternal(name, storedSize);

        if (storedSize != std::tuple_size<T>::value) {
            throw ContainerSizeException("Size of stored array doesn't match provided one.");
        }

        RecursiveVisitor recursiveVisitor(*this, name);
        GetTupleVisitor visitor(recursiveVisitor);
        visitFields(values, &visitor);
    }

    class GetTupleVisitor
    {
    public:
        GetTupleVisitor(RecursiveVisitor& visitor) : mVisitor(visitor) {}

        template<typename T>
        void visit(T& value)
        {
            const std::string k = key(mVisitor.mKeyPrefix, idx);
            if (!mVisitor.mStore.prefixExists(k)) {
                throw InternalIntegrityException("Corrupted list serialization.");
            }
            mVisitor.visitImpl(k, value);

            ++idx;
        }

    private:
        RecursiveVisitor& mVisitor;
        size_t idx = 0;
    };
};

} // namespace internals

} // namespace cargo

#endif // CARGO_SQLITE_INTERNALS_FROM_KVSTORE_VISITOR_BASE_HPP
