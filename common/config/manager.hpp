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
 * @brief   Configuration management functions
 */

#ifndef COMMON_CONFIG_MANAGER_HPP
#define COMMON_CONFIG_MANAGER_HPP

#include "config/to-json-visitor.hpp"
#include "config/from-json-visitor.hpp"
#include "config/is-visitable.hpp"
#include "utils/fs.hpp"

namespace security_containers {
namespace config {

template <class Config>
void loadFromString(const std::string& jsonString, Config& config)
{
    static_assert(isVisitable<Config>::value, "Use CONFIG_REGISTER macro");

    FromJsonVisitor visitor(jsonString);
    config.accept(visitor);
}

template <class Config>
std::string saveToString(const Config& config)
{
    static_assert(isVisitable<Config>::value, "Use CONFIG_REGISTER macro");

    ToJsonVisitor visitor;
    config.accept(visitor);
    return visitor.toString();
}

template <class Config>
void loadFromFile(const std::string& filename, Config& config)
{
    std::string content;
    if (!utils::readFileContent(filename, content)) {
        throw ConfigException("Could not load " + filename);
    }
    loadFromString(content, config);
}

template <class Config>
void saveToFile(const std::string& filename, const Config& config)
{
    const std::string content = saveToString(config);
    if (!utils::saveFileContent(filename, content)) {
        throw ConfigException("Could not save " + filename);
    }
}

} // namespace config
} // namespace security_containers

#endif // COMMON_CONFIG_MANAGER_HPP
