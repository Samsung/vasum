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

    template<typename T, typename std::enable_if<!isVisitable<T>::value, int>::type = 0>
    void setInternal(const std::string& name, const T& value)
    {
        mStore.set(name, value);
    }

    template<typename T, typename std::enable_if<isVisitable<T>::value, int>::type = 0>
    void setInternal(const std::string& name, const T& value)
    {
        ToKVStoreVisitor visitor(*this, name);
        value.accept(visitor);
    }

    template<typename T, typename std::enable_if<isVisitable<T>::value, int>::type = 0>
    void setInternal(const std::string& name, const std::vector<T>& values)
    {
        mStore.remove(name);
        mStore.set(name, values.size());
        for (size_t i = 0; i < values.size(); ++i) {
            setInternal(key(name, std::to_string(i)), values[i]);
        }
    }
};

} // namespace config

#endif // COMMON_CONFIG_TO_KVSTORE_VISITOR_HPP
