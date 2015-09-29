/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Krzysztof Dynowski (k.dynowski@samsumg.com)
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
 * @author  Krzysztof Dynowski (k.dynowski@samsumg.com)
 * @brief   KVStore visitor with defaults values from json
 */

#ifndef COMMON_CONFIG_FROM_KVJSON_VISITOR_HPP
#define COMMON_CONFIG_FROM_KVJSON_VISITOR_HPP

#include "config/from-kvstore-visitor.hpp"
#include "config/is-union.hpp"

#include <json.h>

namespace config {

class FromKVJsonVisitor {
public:
    FromKVJsonVisitor(KVStore& store, const std::string& json, const std::string& prefix)
        : mStore(store)
        , mKeyPrefix(prefix)
        , mIsUnion(false)
    {
        mObject = json_tokener_parse(json.c_str());
        if (mObject == nullptr) {
            throw ConfigException("Json parsing error");
        }
    }


    ~FromKVJsonVisitor() {
        json_object_put(mObject);
    }

    FromKVJsonVisitor(const FromKVJsonVisitor& v) :
        mStore(v.mStore),
        mKeyPrefix(v.mKeyPrefix),
        mObject(v.mObject ? json_object_get(v.mObject) : nullptr),
        mIsUnion(v.mIsUnion)
    {
    }
    FromKVJsonVisitor& operator=(const FromKVJsonVisitor&) = delete;

    template<typename T>
    void visit(const std::string& name, T& value) {
        getValue(name, value);
    }

private:
    KVStore& mStore;
    std::string mKeyPrefix;
    json_object* mObject;
    bool mIsUnion;

    // create visitor for field object (visitable object)
    FromKVJsonVisitor(const FromKVJsonVisitor& v, const std::string& name, const bool isUnion) :
        mStore(v.mStore),
        mKeyPrefix(key(v.mKeyPrefix, name)),
        mIsUnion(isUnion || v.mIsUnion)
    {
        json_object* object = nullptr;
        if (v.mObject && !json_object_object_get_ex(v.mObject, name.c_str(), &object)) {
            if (!mIsUnion)
                throw ConfigException("Missing json field " + mKeyPrefix);
        }
        mObject = object ? json_object_get(object) : nullptr;
    }

    // create visitor for vector i-th element (visitable object)
    FromKVJsonVisitor(const FromKVJsonVisitor& v, int i, const bool isUnion) :
        mStore(v.mStore),
        mKeyPrefix(key(v.mKeyPrefix, std::to_string(i))),
        mIsUnion(isUnion || v.mIsUnion)
    {
        json_object* object = nullptr;
        if (v.mObject) {
            object = json_object_array_get_idx(v.mObject, i);
            checkType(object, json_type_object);
        }
        mObject = object ? json_object_get(object) : nullptr;
    }

    template<typename T, typename std::enable_if<!isVisitable<T>::value, int>::type = 0>
    void getValue(const std::string& name, T& t)
    {
        std::string k = key(mKeyPrefix, name);
        if (mStore.exists(k)) {
            t = mStore.get<T>(k);
        }
        else {
            json_object* object = nullptr;
            if (mObject) {
                json_object_object_get_ex(mObject, name.c_str(), &object);
            }
            if (!object) {
                throw ConfigException("Missing json field " + key(mKeyPrefix, name));
            }
            fromJsonObject(object, t);
        }
    }

    template<typename T, typename std::enable_if<isUnion<T>::value, int>::type = 0>
    void getValue(const std::string& name, T& t)
    {
        FromKVJsonVisitor visitor(*this, name, true);
        t.accept(visitor);
    }

    template<typename T, typename std::enable_if<isVisitable<T>::value && !isUnion<T>::value, int>::type = 0>
    void getValue(const std::string& name, T& t)
    {
        FromKVJsonVisitor visitor(*this, name, false);
        t.accept(visitor);
    }

    int getArraySize(std::string& name, json_object* object)
    {
        if (mStore.exists(name)) {
            return mStore.get<int>(name);
        }
        if (object) {
            return json_object_array_length(object);
        }
        return -1;
    }

    template<typename T>
    void getValue(const std::string& name, std::vector<T>& value)
    {
        json_object* object = nullptr;
        if (mObject && json_object_object_get_ex(mObject, name.c_str(), &object)) {
            checkType(object, json_type_array);
        }

        std::string k = key(mKeyPrefix, name);
        int length = getArraySize(k, object);
        if (length < 0) {
            throw ConfigException("Missing array length " + k);
        }
        value.resize(static_cast<size_t>(length));
        FromKVJsonVisitor visitor(*this, name, false);
        if (mStore.exists(k)) {
            json_object_put(visitor.mObject);
            visitor.mObject = nullptr;
        }
        for (int i = 0; i < length; ++i) {
            visitor.getValue(i, value[i]);
        }
    }

    template<typename T, std::size_t N>
    void getValue(const std::string& name, std::array<T, N>& values)
    {
        json_object* object = nullptr;
        if (mObject && json_object_object_get_ex(mObject, name.c_str(), &object)) {
            checkType(object, json_type_array);
        }

        std::string k = key(mKeyPrefix, name);
        FromKVJsonVisitor visitor(*this, name, false);
        if (mStore.exists(k)) {
            json_object_put(visitor.mObject);
            visitor.mObject = nullptr;
        }
        for (std::size_t i = 0; i < N; ++i) {
            visitor.getValue(i, values[i]);
        }
    }

    template<typename T, typename std::enable_if<!isVisitable<T>::value, int>::type = 0>
    void getValue(int i, T& t)
    {
        std::string k = key(mKeyPrefix, std::to_string(i));
        if (mStore.exists(k)) {
            t = mStore.get<T>(k);
        }
        else {
            json_object* object = mObject ? json_object_array_get_idx(mObject, i) : nullptr;
            if (!object) {
                throw ConfigException("Missing json array elem " + k);
            }
            fromJsonObject(object, t);
        }
    }

    template<typename T, typename std::enable_if<isUnion<T>::value, int>::type = 0>
    void getValue(int i, T& t)
    {
        FromKVJsonVisitor visitor(*this, i, true);
        t.accept(visitor);
    }

    template<typename T, typename std::enable_if<isVisitable<T>::value && !isUnion<T>::value, int>::type = 0>
    void getValue(int i, T& t)
    {
        FromKVJsonVisitor visitor(*this, i, false);
        t.accept(visitor);
    }

    template<typename T>
    void getValue(int i, std::vector<T>& value)
    {
        std::string k = key(mKeyPrefix, std::to_string(i));
        int length = getArraySize(k, mObject);
        value.resize(static_cast<size_t>(length));
        FromKVJsonVisitor visitor(*this, i, false);
        if (mStore.exists(k)) {
            json_object_put(visitor.mObject);
            visitor.mObject = nullptr;
        }
        for (int idx = 0; idx < length; ++idx) {
            visitor.getValue(idx, value[idx]);
        }
    }

    template<typename T, std::size_t N>
    void getValue(int i, std::array<T, N>& values)
    {
        std::string k = key(mKeyPrefix, std::to_string(i));

        FromKVJsonVisitor visitor(*this, i, false);
        if (mStore.exists(k)) {
            json_object_put(visitor.mObject);
            visitor.mObject = nullptr;
        }
        for (std::size_t idx = 0; idx < N; ++idx) {
            visitor.getValue(idx, values[idx]);
        }
    }

    static void checkType(json_object* object, json_type type)
    {
        if (type != json_object_get_type(object)) {
            throw ConfigException("Invalid field type " + std::to_string(type));
        }
    }

    static void fromJsonObject(json_object* object, int& value)
    {
        checkType(object, json_type_int);
        std::int64_t value64 = json_object_get_int64(object);
        if (value64 > INT32_MAX || value64 < INT32_MIN) {
            throw ConfigException("Value out of range");
        }
        value = static_cast<int>(value64);
    }

    static void fromJsonObject(json_object* object, std::int64_t& value)
    {
        checkType(object, json_type_int);
        value = json_object_get_int64(object);
    }

    static void fromJsonObject(json_object* object, std::uint32_t& value)
    {
        checkType(object, json_type_int);
        std::int64_t value64 = json_object_get_int64(object);
        if (value64 > UINT32_MAX || value64 < 0) {
            throw ConfigException("Value out of range");
        }
        value = static_cast<std::uint32_t>(value64);
    }

    static void fromJsonObject(json_object* object, std::uint64_t& value)
    {
        checkType(object, json_type_int);
        std::int64_t value64 = json_object_get_int64(object);
        if (value64 < 0) {
            throw ConfigException("Value out of range");
        }
        value = static_cast<std::uint64_t>(value64);
    }

    static void fromJsonObject(json_object* object, bool& value)
    {
        checkType(object, json_type_boolean);
        value = json_object_get_boolean(object);
    }

    static void fromJsonObject(json_object* object, double& value)
    {
        checkType(object, json_type_double);
        value = json_object_get_double(object);
    }

    static void fromJsonObject(json_object* object, std::string& value)
    {
        checkType(object, json_type_string);
        value = json_object_get_string(object);
    }

    static void fromJsonObject(json_object* object, char* &value)
    {
        checkType(object, json_type_string);

        int len = json_object_get_string_len(object);
        value = new char[len + 1];
        std::strncpy(value, json_object_get_string(object), len);
        value[len] = '\0';
    }
};

} // namespace config

#endif // COMMON_CONFIG_FROM_KVJSON_VISITOR_HPP
