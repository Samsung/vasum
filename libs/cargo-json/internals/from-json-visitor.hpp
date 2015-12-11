/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Piotr Bartosiewicz (p.bartosiewi@partner.samsung.com)
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
 * @author  Piotr Bartosiewicz (p.bartosiewi@partner.samsung.com)
 * @brief   JSON visitor
 */

#ifndef CARGO_JSON_INTERNALS_FROM_JSON_VISITOR_HPP
#define CARGO_JSON_INTERNALS_FROM_JSON_VISITOR_HPP

#include "cargo/internals/is-visitable.hpp"
#include "cargo/internals/is-like-tuple.hpp"
#include "cargo/exception.hpp"
#include "cargo/internals/visit-fields.hpp"

#include <json.h>
#include <string>
#include <cstring>
#include <vector>
#include <array>
#include <utility>
#include <limits>

namespace cargo {

namespace internals {

class FromJsonVisitor {
public:
    explicit FromJsonVisitor(const std::string& jsonString)
        : mObject(nullptr)
    {
        mObject = json_tokener_parse(jsonString.c_str());
        if (mObject == nullptr) {
            throw CargoException("Json parsing error");
        }
    }

    FromJsonVisitor(const FromJsonVisitor& visitor)
        : mObject(json_object_get(visitor.mObject))
    {
    }

    ~FromJsonVisitor()
    {
        json_object_put(mObject);
    }

    FromJsonVisitor& operator=(const FromJsonVisitor&) = delete;

    template<typename T>
    void visit(const std::string& name, T& value)
    {
        json_object* object = nullptr;
        if (!json_object_object_get_ex(mObject, name.c_str(), &object)) {
            throw CargoException("Missing field '" + name + "'");
        }
        fromJsonObject(object, value);
    }

private:
    json_object* mObject;


    explicit FromJsonVisitor(json_object* object)
        : mObject(json_object_get(object))
    {
    }

    static void checkType(json_object* object, json_type type)
    {
        if (type != json_object_get_type(object)) {
            throw CargoException("Invalid field type");
        }
    }

    template<typename T, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
    static void fromJsonObject(json_object* object, T& value)
    {
        checkType(object, json_type_int);
        typedef typename std::conditional<std::is_signed<T>::value,
                                          std::int64_t, std::uint64_t>::type BufferType;
        BufferType value64 = json_object_get_int64(object);
        if (value64 > std::numeric_limits<T>::max() || value64 < std::numeric_limits<T>::min()) {
            throw CargoException("Value out of range");
        }
        value = static_cast<T>(value64);
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

    template<typename T>
    static void fromJsonObject(json_object* object, std::vector<T>& values)
    {
        checkType(object, json_type_array);
        int length = json_object_array_length(object);
        values.resize(static_cast<size_t>(length));
        for (int i = 0; i < length; ++i) {
            fromJsonObject(json_object_array_get_idx(object, i), values[static_cast<size_t>(i)]);
        }
    }

    template<typename T, std::size_t N>
    static void fromJsonObject(json_object* object, std::array<T, N>& values)
    {
        checkType(object, json_type_array);

        for (std::size_t i = 0; i < N; ++i) {
            fromJsonObject(json_object_array_get_idx(object, i), values[i]);
        }
    }

    template<typename V>
    static void fromJsonObject(json_object* object, std::map<std::string, V>& values)
    {
        checkType(object, json_type_object);

        json_object_object_foreach(object, key, val) {
            fromJsonObject(val, values[key]);
        }
    }

    struct HelperVisitor
    {
        template<typename T>
        static void visit(json_object* object, std::size_t& idx, T&& value)
        {
            fromJsonObject(json_object_array_get_idx(object, idx), value);
            idx += 1;
        }
    };

    template<typename T, typename std::enable_if<isLikeTuple<T>::value, int>::type = 0>
    static void fromJsonObject(json_object* object, T& values)
    {
        checkType(object, json_type_array);

        std::size_t idx = 0;
        HelperVisitor visitor;
        visitFields(values, &visitor, object, idx);
    }

    template<typename T, typename std::enable_if<std::is_enum<T>::value, int>::type = 0>
    static void fromJsonObject(json_object* object, T& value)
    {
        fromJsonObject(object,
                       *reinterpret_cast<typename std::underlying_type<T>::type*>(&value));
    }

    template<typename T, typename std::enable_if<isVisitable<T>::value, int>::type = 0>
    static void fromJsonObject(json_object* object, T& value)
    {
        checkType(object, json_type_object);
        FromJsonVisitor visitor(object);
        value.accept(visitor);
    }
};

} // namespace internals

} // namespace cargo

#endif // CARGO_JSON_INTERNALS_FROM_JSON_VISITOR_HPP
