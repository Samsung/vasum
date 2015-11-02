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

#ifndef COMMON_CONFIG_FROM_KVSTORE_VISITOR_BASE_HPP
#define COMMON_CONFIG_FROM_KVSTORE_VISITOR_BASE_HPP

#include "config/exception.hpp"
#include "config/is-visitable.hpp"
#include "config/kvstore.hpp"
#include "config/kvstore-visitor-utils.hpp"
#include "config/visit-fields.hpp"


namespace config {

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
    template<typename T,
             typename std::enable_if<!isVisitable<T>::value
                                  && !std::is_enum<T>::value, int>::type = 0>
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

    template<typename ... T>
    void getInternal(const std::string& key, std::pair<T...>& values)
    {
        std::vector<std::string> strValues;

        getInternal(key, strValues);
        if (strValues.empty()) {
            return;
        }

        if (strValues.size() != sizeof...(T)) {
            throw ContainerSizeException("Size of stored tuple doesn't match provided one.");
        }

        GetTupleVisitor visitor;
        visitFields(values, &visitor, strValues.begin());
    }
};

} // namespace config

#endif // COMMON_CONFIG_FROM_KVSTORE_VISITOR_BASE_HPP
