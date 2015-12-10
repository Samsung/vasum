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
 * @brief   Filesystem helper functions
 */

#include "config.hpp"

#include "cargo-json/internals/fs-utils.hpp"

#include <fstream>
#include <streambuf>


namespace cargo {
namespace internals {
namespace fsutils {

bool readFileContent(const std::string& path, std::string& result)
{
    std::ifstream file(path);

    if (!file) {
        return false;
    }

    file.seekg(0, std::ios::end);
    std::streampos length = file.tellg();
    if (length < 0) {
        return false;
    }
    result.resize(static_cast<size_t>(length));
    file.seekg(0, std::ios::beg);

    file.read(&result[0], length);
    if (!file) {
        result.clear();
        return false;
    }

    return true;
}

bool saveFileContent(const std::string& path, const std::string& content)
{
    std::ofstream file(path);
    if (!file) {
        return false;
    }
    file.write(content.data(), static_cast<std::streamsize>(content.size()));
    if (!file) {
        return false;
    }
    return true;
}

} // namespace fsutils
} // namespace internals
} // namespace cargo
