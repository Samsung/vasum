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
 * @author  Pawel Kubik (p.kubik@samsung.com)
 * @brief   KVStore visitors utilities
 */

#ifndef CARGO_SQLITE_INTERNALS_KVSTORE_VISITOR_UTILS_HPP
#define CARGO_SQLITE_INTERNALS_KVSTORE_VISITOR_UTILS_HPP

#include <vector>
#include <string>
#include <sstream>

namespace cargo {

namespace internals {

template<typename T>
T fromString(const std::string& strValue)
{
    std::istringstream iss(strValue);
    T value;
    iss >> value;
    return value;
}

template<typename T>
std::string toString(const T& value)
{
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

/**
 * Concatenates all parameters into one std::string.
 * Uses '.' to connect the terms.
 * @param args components of the string
 * @tparam delim optional delimiter
 * @tparam Args any type implementing str
 * @return string created from he args
 */
template<char delim = '.', typename Arg1, typename ... Args>
std::string key(const Arg1& a1, const Args& ... args)
{
    std::string ret = toString(a1);
    std::initializer_list<std::string> strings {toString(args)...};
    for (const std::string& s : strings) {
        ret += delim + s;
    }

    return ret;
}

/**
 * Function added for key function completeness.
 *
 * @tparam delim = '.' parameter not used, added for consistency
 * @return empty string
 */
template<char delim = '.'>
std::string key()
{
    return std::string();
}

} // namespace internals

} // namespace cargo

#endif // CARGO_SQLITE_INTERNALS_KVSTORE_VISITOR_UTILS_HPP
