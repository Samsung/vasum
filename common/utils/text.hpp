/*
 *  Copyright (C) 2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License version 2.1 as published by the Free Software Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
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

std::string join(const std::vector<std::string>& vec, const char *delim);
std::vector<std::string> split(const std::string& str, const std::string& delim);

} // namespace utils

#endif // COMMON_UTILS_TEXT_HPP
