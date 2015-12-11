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

#ifndef CARGO_JSON_INTERNALS_TO_JSON_VISITOR_HPP
#define CARGO_JSON_INTERNALS_TO_JSON_VISITOR_HPP

#include "cargo/internals/is-visitable.hpp"
#include "cargo/internals/is-like-tuple.hpp"
#include "cargo/exception.hpp"
#include "cargo/internals/visit-fields.hpp"

#include <array>
#include <string>
#include <vector>
#include <map>
#include <utility>

#include <json.h>

namespace cargo {

namespace internals {

class ToJsonVisitor {

public:
    ToJsonVisitor()
        : mObject(json_object_new_object())
    {
    }

    ToJsonVisitor(const ToJsonVisitor& visitor)
        : mObject(json_object_get(visitor.mObject))
    {
    }

    ~ToJsonVisitor()
    {
        json_object_put(mObject);
    }

    ToJsonVisitor& operator=(const ToJsonVisitor&) = delete;

    std::string toString() const
    {
        return json_object_to_json_string(mObject);
    }

    template<typename T>
    void visit(const std::string& name, const T& value)
    {
        json_object_object_add(mObject, name.c_str(), toJsonObject(value));
    }
private:
    json_object* mObject;


    json_object* detach()
    {
        json_object* ret = mObject;
        mObject = nullptr;
        return ret;
    }

    template<typename T, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
    static json_object* toJsonObject(T value)
    {
        return toJsonObject(static_cast<int64_t>(value));
    }

    static json_object* toJsonObject(std::int64_t value)
    {
        return json_object_new_int64(value);
    }

    static json_object* toJsonObject(std::uint64_t value)
    {
        if (value > INT64_MAX) {
            throw CargoException("Value out of range");
        }
        return json_object_new_int64(value);
    }

    static json_object* toJsonObject(bool value)
    {
        return json_object_new_boolean(value);
    }

    static json_object* toJsonObject(double value)
    {
#if JSON_C_MINOR_VERSION >= 12
        return json_object_new_double_s(value, std::to_string(value).c_str());
#else
        return json_object_new_double(value);
#endif
    }

    static json_object* toJsonObject(const std::string& value)
    {
        return json_object_new_string(value.c_str());
    }

    static json_object* toJsonObject(char* value)
    {
        return json_object_new_string(value);
    }

    template<typename T>
    static json_object* toJsonObject(const std::vector<T>& value)
    {
        json_object* array = json_object_new_array();
        for (const T& v : value) {
            json_object_array_add(array, toJsonObject(v));
        }
        return array;
    }

    template<typename T, std::size_t N>
    static json_object* toJsonObject(const std::array<T, N>& values)
    {
        json_object* array = json_object_new_array();
        for (const T& v : values) {
            json_object_array_add(array, toJsonObject(v));
        }
        return array;
    }

    template<typename V>
    static json_object* toJsonObject(const std::map<std::string, V>& values)
    {
        json_object* newMap = json_object_new_object();
        for (const auto& item : values) {
            json_object_object_add(newMap, item.first.c_str(), toJsonObject(item.second));
        }
        return newMap;
    }

    struct HelperVisitor
    {
        template<typename T>
        static void visit(json_object* array, const T& value)
        {
            json_object_array_add(array, toJsonObject(value));
        }
    };

    template<typename T, typename std::enable_if<isLikeTuple<T>::value, int>::type = 0>
    static json_object* toJsonObject(const T& values)
    {
        json_object* array = json_object_new_array();

        HelperVisitor visitor;
        visitFields(values, &visitor, array);
        return array;
    }

    template<typename T, typename std::enable_if<isVisitable<T>::value, int>::type = 0>
    static json_object* toJsonObject(const T& value)
    {
        ToJsonVisitor visitor;
        value.accept(visitor);
        return visitor.detach();
    }

    template<typename T, typename std::enable_if<std::is_enum<T>::value, int>::type = 0>
    static json_object* toJsonObject(const T& value)
    {
        return toJsonObject(static_cast<const typename std::underlying_type<T>::type>(value));
    }

};

} // namespace internals

} // namespace cargo

#endif // CARGO_JSON_INTERNALS_TO_JSON_VISITOR_HPP
