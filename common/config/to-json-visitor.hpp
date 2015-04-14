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

#ifndef COMMON_CONFIG_TO_JSON_VISITOR_HPP
#define COMMON_CONFIG_TO_JSON_VISITOR_HPP

#include "config/is-visitable.hpp"
#include "config/exception.hpp"

#include <json/json.h>
#include <string>
#include <vector>

namespace config {

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

    static json_object* toJsonObject(std::int32_t value)
    {
        return json_object_new_int(value);
    }

    static json_object* toJsonObject(std::int64_t value)
    {
        return json_object_new_int64(value);
    }

    static json_object* toJsonObject(std::uint32_t value)
    {
        if (value > INT32_MAX) {
            throw ConfigException("Value out of range");
        }
        return json_object_new_int(value);
    }

    static json_object* toJsonObject(std::uint64_t value)
    {
        if (value > INT64_MAX) {
            throw ConfigException("Value out of range");
        }
        return json_object_new_int64(value);
    }

    static json_object* toJsonObject(bool value)
    {
        return json_object_new_boolean(value);
    }

    static json_object* toJsonObject(double value)
    {
        return json_object_new_double(value);
    }

    static json_object* toJsonObject(const std::string& value)
    {
        return json_object_new_string(value.c_str());
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

    template<typename T, class = typename std::enable_if<isVisitable<T>::value>::type>
    static json_object* toJsonObject(const T& value)
    {
        ToJsonVisitor visitor;
        value.accept(visitor);
        return visitor.detach();
    }
};

} // namespace config

#endif // COMMON_CONFIG_TO_JSON_VISITOR_HPP
