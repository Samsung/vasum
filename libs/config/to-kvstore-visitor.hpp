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

#ifndef COMMON_CONFIG_TO_KVSTORE_VISITOR_HPP
#define COMMON_CONFIG_TO_KVSTORE_VISITOR_HPP

#include "config/is-visitable.hpp"
#include "config/kvstore.hpp"
#include "config/kvstore-visitor-utils.hpp"

namespace config {

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

    template<typename T,
             typename std::enable_if<!isVisitable<T>::value
                                  && !std::is_enum<T>::value, int>::type = 0>
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
        if (size > std::numeric_limits<unsigned int>::max()) {
            throw ConfigException("Too many values to insert");
        }

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

    template<typename T>
    void setInternal(const std::string& name, const std::initializer_list<T>& values)
    {
        setRangeInternal(name, values.begin(), values.end(), values.size());
    }

    template<typename ... T>
    void setInternal(const std::string& key, const std::pair<T...>& values)
    {
        std::vector<std::string> strValues(std::tuple_size<std::pair<T...>>::value);

        SetTupleVisitor visitor;
        visitFields(values, &visitor, strValues.begin());
        setInternal(key, strValues);
    }
};

} // namespace config

#endif // COMMON_CONFIG_TO_KVSTORE_VISITOR_HPP
