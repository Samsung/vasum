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

#ifndef COMMON_CONFIG_FROM_JSON_VISITOR_HPP
#define COMMON_CONFIG_FROM_JSON_VISITOR_HPP

#include "config/is-visitable.hpp"
#include "config/exception.hpp"

#include <json/json.h>
#include <string>
#include <vector>

namespace security_containers {
namespace config {

class FromJsonVisitor {
public:
    explicit FromJsonVisitor(const std::string& jsonString)
        : mObject(nullptr)
    {
        mObject = json_tokener_parse(jsonString.c_str());
        if (mObject == nullptr) {
            throw ConfigException("Json parsing error");
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

    template<class T>
    void visit(const std::string& name, T& value)
    {
        json_object* object = nullptr;
        if (!json_object_object_get_ex(mObject, name.c_str(), &object)) {
            throw ConfigException("Missing field '" + name + "' in json");
        }
        fromJsonObject(object, value);
    }

private:
    json_object* mObject;

    FromJsonVisitor& operator=(const FromJsonVisitor&) = delete;

    explicit FromJsonVisitor(json_object* object)
        : mObject(json_object_get(object))
    {
    }

    static void checkType(json_object* object, json_type type)
    {
        if (type != json_object_get_type(object)) {
            throw ConfigException("Invalid field type");
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

    template<class T>
    static void fromJsonObject(json_object* object, std::vector<T>& value)
    {
        checkType(object, json_type_array);
        int length = json_object_array_length(object);
        value.resize(static_cast<size_t>(length));
        for (int i = 0; i < length; ++i) {
            fromJsonObject(json_object_array_get_idx(object, i), value[static_cast<size_t>(i)]);
        }
    }

    template<class T, class = typename std::enable_if<isVisitable<T>::value>::type>
    static void fromJsonObject(json_object* object, T& value)
    {
        checkType(object, json_type_object);
        FromJsonVisitor visitor(object);
        value.accept(visitor);
    }
};

} // namespace config
} // namespace security_containers

#endif // COMMON_CONFIG_FROM_JSON_VISITOR_HPP
