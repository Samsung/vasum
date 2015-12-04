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
 * @brief   src/cargo/manager.hpp
 */

#ifndef CARGO_JSON_INTERNALS_FS_UTILS_HPP
#define CARGO_JSON_INTERNALS_FS_UTILS_HPP

#include <string>

namespace cargo {
namespace internals {
namespace fsutils {

bool readFileContent(const std::string& path, std::string& result);
bool saveFileContent(const std::string& path, const std::string& content);

inline std::string readFileContent(const std::string& path) {
    std::string content;
    return readFileContent(path, content) ? content : std::string();
}
} // namespace fsutils
} // namespace internals
} // namespace cargo

#endif // CARGO_JSON_INTERNALS_FS_UTILS_HPP
