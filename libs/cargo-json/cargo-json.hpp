/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Pawel Kubik (p.kubik@samsung.com)
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
 * @author   Pawel Kubik (p.kubik@samsung.com)
 * @defgroup libcargo-json libcargo-json
 * @brief    cargo Json interface
 */

#ifndef CARGO_JSON_CARGO_JSON_HPP
#define CARGO_JSON_CARGO_JSON_HPP

#include "cargo-json/internals/to-json-visitor.hpp"
#include "cargo-json/internals/from-json-visitor.hpp"
#include "cargo-json/internals/fs-utils.hpp"
#include "logger/logger.hpp"

namespace cargo {

/*@{*/

/**
 * Fills the visitable with data stored in the json string
 *
 * @param jsonString    data in a json format
 * @param visitable        visitable structure to fill
 */
template <class Cargo>
void loadFromJsonString(const std::string& jsonString, Cargo& visitable)
{
    static_assert(isVisitable<Cargo>::value, "Use CARGO_REGISTER macro");

    FromJsonVisitor visitor(jsonString);
    visitable.accept(visitor);
}

/**
 * Creates a string representation of the visitable in json format
 *
 * @param visitable   visitable structure to convert
 */
template <class Cargo>
std::string saveToJsonString(const Cargo& visitable)
{
    static_assert(isVisitable<Cargo>::value, "Use CARGO_REGISTER macro");

    ToJsonVisitor visitor;
    visitable.accept(visitor);
    return visitor.toString();
}

/**
 * Loads the visitable from a json file
 *
 * @param filename    path to the file
 * @param visitable   visitable structure to load
 */
template <class Cargo>
void loadFromJsonFile(const std::string& filename, Cargo& visitable)
{
    std::string content;
    if (!fsutils::readFileContent(filename, content)) {
        const std::string& msg = "Could not load " + filename;
        LOGE(msg);
        throw CargoException(msg);
    }
    try {
        loadFromJsonString(content, visitable);
    } catch (CargoException& e) {
        const std::string& msg = "Error in " + filename + ": " + e.what();
        LOGE(msg);
        throw CargoException(msg);
    }
}

/**
 * Saves the visitable in a json file
 *
 * @param filename    path to the file
 * @param visitable   visitable structure to save
 */
template <class Cargo>
void saveToJsonFile(const std::string& filename, const Cargo& visitable)
{
    const std::string content = saveToJsonString(visitable);
    if (!fsutils::saveFileContent(filename, content)) {
        throw CargoException("Could not save " + filename);
    }
}

} // namespace cargo

/*@}*/

#endif // CARGO_JSON_CARGO_JSON_HPP
