/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Michal Witanowski <m.witanowski@samsung.com>
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
 *  limitations under the License.
 */


/**
 * @file
 * @author  Michal Witanowski (m.witanowski@samsung.com)
 * @brief   Helper class for parsing and storing configurations in JSON format.
 */

#ifndef COMMON_CONFIG_CONFIGURATION_HPP
#define COMMON_CONFIG_CONFIGURATION_HPP

#include "log/logger.hpp"
#include "config/exception.hpp"

#include <json/json.h>
#include <string>
#include <vector>


namespace security_containers {
namespace config {


enum class ConfigProcessMode : int {
    Read,
    Write
};

#define CONFIG_REGISTER \
    void process(json_object* obj, security_containers::config::ConfigProcessMode mode)

/**
 * Use this macro to declare config value (class attribute). Declare means that it will be
 * read / written when calling ConfigurationBase::parseStr() or ConfigurationBase::toString().
 * Currently supported types are:
 * int, bool, double, std::string and std::vector of all the mentioned.
 */
#define CONFIG_VALUE(name) \
    if (mode == security_containers::config::ConfigProcessMode::Read) \
        readValue(obj, name, #name);                                  \
    else                                                              \
        writeValue(obj, name, #name);
/**
 * Use this macro to declare configuration sub object (nested config tree).
 * The type of passed argument must inherit from ConfigurationBase or be std::vector of such type.
 */
#define CONFIG_SUB_OBJECT(name) \
    if (mode == security_containers::config::ConfigProcessMode::Read) \
        readSubObj(obj, name, #name);                                 \
    else                                                              \
        writeSubObj(obj, name, #name);

/**
    @brief Override this abstract class to enable reading/writing a class members from/to JSON
    format. Use the macros above to define config class. For example:

    struct Foo : public ConfigurationBase
    {
        std::string bar;
        std::vector<int> tab;

        // SubConfigA must derive from GenericConfig
        SubConfigA sub_a;

        // SubConfigB must derive from GenericConfig
        std::vector<SubConfigB> tab_sub;

        CONFIG_REGISTER
        {
            CONFIG_VALUE(bar)
            CONFIG_VALUE(tab)
            CONFIG_SUB_OBJECT(sub_a)
        }
    };
    // ...
    Foo config;
    config.parseFile("file.path");
    // use parsed object:
    std::cout << config.bar;
*/
class ConfigurationBase {
public:
    virtual ~ConfigurationBase() {}

    /**
     * Parse config object from a string.
     * This method throws ConfigException when fails.
     * @param   str      JSON string to be parsed
     */
    void parseStr(const std::string& str);

    /**
     * Parse config object from a file.
     * This method throws ConfigException when fails.
     * @param   path        Source file path
     */
    void parseFile(const std::string& path);

    /**
     * Convert cofig object to string.
     * This method throws ConfigException when fails.
     * @return  String in JSON format
     */
    std::string toString() const;

    /**
     * Write cofig object to a file (in JSON format).
     * This method throws ConfigException when fails.
     * @param  path        Target file path
     */
    void saveToFile(const std::string& path) const;

protected:
    /**
     * Virtual method called while reading or writing config from/to JSON format.
     * Override it using CONFIG_REGISTER macro (see an example above)
     * @param   jsonObj     Current JSON tree node
     * @param   mode        Reading from jsonObj vs. writing to jsonObj
     */
    virtual void process(json_object* jsonObj, ConfigProcessMode mode) = 0;


    // template for reading single values from JSON tree
    template <typename T>
    static void readValue(json_object* jsonObj, T& val, const char* name)
    {
        json_object* obj = NULL;
        if (!json_object_object_get_ex(jsonObj, name, &obj)) {
            LOGE("json_object_object_get_ex() failed, name = " << name);
            throw ConfigException();
        }
        val = getValueFromJsonObj<T>(obj);
    }

    // template for writing single values to JSON tree
    template <typename T>
    static void writeValue(json_object* jsonObj, T& val, const char* name)
    {
        json_object_object_add(jsonObj, name, getJsonObjFromValue<T>(val));
    }

    // template for reading arrays (STL vectors) from JSON tree
    template <typename T>
    static void readValue(json_object* jsonObj, std::vector<T>& val, const char* name)
    {
        val.clear();
        json_object* array = NULL;
        if (!json_object_object_get_ex(jsonObj, name, &array)) {
            LOGE("json_object_object_get_ex() failed, name = " << name);
            throw ConfigException();
        }
        int len = json_object_array_length(array);
        val.resize(len);
        for (int i = 0; i < len; ++i) {
            val[i] = getValueFromJsonObj<T>(json_object_array_get_idx(array, i));
        }
    }

    // template for writing arrays to JSON tree
    template <typename T>
    static void writeValue(json_object* jsonObj, std::vector<T>& val, const char* name)
    {
        json_object* array = json_object_new_array();
        if (NULL == array) {
            LOGE("json_object_new_array() failed, name = " << name);
            throw ConfigException();
        }
        for (auto& el : val) {
            json_object_array_add(array, getJsonObjFromValue<T>(el));
        }
        json_object_object_add(jsonObj, name, array);
    }

    // ============================================================================================

    // template for reading sub-classes
    template <typename T>
    static void readSubObj(json_object* jsonObj, T& val, const char* name)
    {
        json_object* obj = NULL;
        if (!json_object_object_get_ex(jsonObj, name, &obj)) {
            LOGE("json_object_object_get_ex() failed, name = " << name);
            throw ConfigException();
        }
        val.process(obj, ConfigProcessMode::Read);
    }

    // template for writing sub-classes
    template <typename T>
    static void writeSubObj(json_object* jsonObj, T& val, const char* name)
    {
        json_object* obj = json_object_new_object();
        if (!obj) {
            LOGE("json_object_new_object() failed, name = " << name);
            throw ConfigException();
        }
        val.process(obj, ConfigProcessMode::Write);
        json_object_object_add(jsonObj, name, obj);
    }

    // template for reading array of sub-classes from JSON tree
    template <typename T>
    static void readSubObj(json_object* jsonObj, std::vector<T>& val, const char* name)
    {
        val.clear();
        json_object* obj = NULL;
        if (!json_object_object_get_ex(jsonObj, name, &obj)) {
            LOGE("json_object_object_get_ex() failed, name = " << name);
            throw ConfigException();
        }

        int len = json_object_array_length(obj);
        val.resize(len);
        for (int i = 0; i < len; ++i) {
            json_object* arrayObj = json_object_array_get_idx(obj, i);
            val[i].process(arrayObj, ConfigProcessMode::Read);
        }
    }

    // template for writing array of sub-classes to JSON tree
    template <typename T>
    static void writeSubObj(json_object* jsonObj, std::vector<T>& val, const char* name)
    {
        json_object* array = json_object_new_array();
        if (!array) {
            LOGE("json_object_new_array() failed, name = " << name);
            throw ConfigException();
        }

        for (auto& element : val) {
            json_object* obj = json_object_new_object();
            if (!obj) {
                LOGE("json_object_new_object() failed, name = " << name);
                throw ConfigException();
            }
            element.process(obj, ConfigProcessMode::Write);
            json_object_array_add(array, obj);
        }
        json_object_object_add(jsonObj, name, array);
    }

private:
    // template used to convert JSON tree node into simple C++ type
    template <typename T>
    static T getValueFromJsonObj(json_object* obj);

    // template used to convert simple C++ type into JSON tree node
    template <typename T>
    static json_object* getJsonObjFromValue(const T& val);
};


} // namespace config
} // namespace security_containers


#endif // COMMON_CONFIG_CONFIGURATION_HPP
