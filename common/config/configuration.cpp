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

#include "log/logger.hpp"
#include "config/configuration.hpp"
#include "config/exception.hpp"

#include <fstream>


namespace security_containers {
namespace config {


void ConfigurationBase::parseStr(const std::string& str)
{
    json_tokener_error err;
    json_object* obj = json_tokener_parse_verbose(str.c_str(), &err);
    if (!obj) {
        LOGE("Error during parsing configuration");
        throw ConfigException();
    }

    process(obj, ConfigProcessMode::Read);
    json_object_put(obj); // free memory
}

void ConfigurationBase::parseFile(const std::string& path)
{
    std::ifstream file(path, std::ifstream::in);
    if (!file.good()) {
        LOGE("Error during opening configuration file");
        throw ConfigException();
    }

    // read file to std::string
    std::string str((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    parseStr(str.c_str());
}

std::string ConfigurationBase::toString() const
{
    json_object* obj = json_object_new_object();
    if (!obj) {
        LOGE("json_object_new_object failed");
        throw ConfigException();
    }

    // process() is not const (it's used to read/write) so we need to drop const form "this"
    const_cast<ConfigurationBase*>(this)->process(obj, ConfigProcessMode::Write);
    std::string result = std::string(json_object_to_json_string(obj));

    json_object_put(obj); // free memory
    return result;
}

void ConfigurationBase::saveToFile(const std::string& path) const
{
    std::ofstream file(path);
    if (!file.good()) {
        LOGE("Error during saving configuration file");
        throw ConfigException();
    }

    file << toString();
}


// ------------------------------------------------------------------------------------------------
// template specializations for writing simple types

template<> json_object* ConfigurationBase::getJsonObjFromValue(const int& val)
{
    return json_object_new_int(val);
}

template<> json_object* ConfigurationBase::getJsonObjFromValue(const bool& val)
{
    return json_object_new_boolean(val);
}

template<> json_object* ConfigurationBase::getJsonObjFromValue(const double& val)
{
    return json_object_new_double(val);
}

template<> json_object* ConfigurationBase::getJsonObjFromValue(const std::string& val)
{
    return json_object_new_string(val.c_str());
}


// ------------------------------------------------------------------------------------------------
// template specializations for reading simple types

template<> int ConfigurationBase::getValueFromJsonObj(json_object* obj)
{
    return json_object_get_int(obj);
}

template<> bool ConfigurationBase::getValueFromJsonObj(json_object* obj)
{
    return json_object_get_boolean(obj);
}

template<> double ConfigurationBase::getValueFromJsonObj(json_object* obj)
{
    return json_object_get_double(obj);
}

template<> std::string ConfigurationBase::getValueFromJsonObj(json_object* obj)
{
    return std::string(json_object_get_string(obj));
}

} // namespace config
} // namespace security_containers
