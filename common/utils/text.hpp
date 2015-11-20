/*
 *  Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Krzysztof Dynowski (k.dynowski@samsumg.com)
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
 * @author  Krzysztof Dynowski (k.dynowski@samsumg.com)
 * @brief   Text related utils
 */

#ifndef COMMON_UTILS_TEXT_HPP
#define COMMON_UTILS_TEXT_HPP

#include <string>
#include <vector>
#include <sstream>

namespace utils {

inline bool beginsWith(std::string const &value, std::string const &part)
{
     if (part.size() > value.size()) {
         return false;
     }
     return std::equal(part.begin(), part.end(), value.begin());
}

inline bool endsWith(std::string const &value, std::string const &part)
{
     if (part.size() > value.size()) {
         return false;
     }
     return std::equal(part.rbegin(), part.rend(), value.rbegin());
}

/**
 * Convert binary bytes array to hex string representation
 */
std::string toHexString(const void *data, unsigned len);

template<typename T>
std::string join(const std::vector<T>& vec, const char *delim)
{
    std::stringstream res;
    for (const auto& s : vec) {
        if (res.tellp()>0) {
            res << delim;
        }
        res << s;
    }
    return res.str();
}

std::vector<std::string> split(const std::string& str, const std::string& delim);

} // namespace utils

#endif // COMMON_UTILS_TEXT_HPP
