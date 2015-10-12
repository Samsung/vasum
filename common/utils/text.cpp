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
 * @brief   Text related utility
 */

#include "utils/text.hpp"
#include <sstream>

namespace utils {
namespace {
const char hexmap[] = {'0', '1', '2', '3', '4', '5', '6', '7',
                       '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
}

std::string toHexString(const void *data, unsigned len)
{
    const unsigned char *d = static_cast<const unsigned char *>(data);
    std::string s(len * 2, ' ');
    for (unsigned i = 0; i < len; ++i) {
        s[2 * i]     = hexmap[(d[i] >> 4) & 0x0F];
        s[2 * i + 1] = hexmap[d[i] & 0x0F];
    }
    return s;
}

std::string join(const std::vector<std::string>& vec, const char *delim)
{
    std::stringstream res;
    for (const auto& s : vec) {
        if (res.tellp() > 0) {
            res << delim;
        }
        res << s;
    }
    return res.str();
}

std::vector<std::string> split(const std::string& str, const std::string& delim)
{
    std::vector<std::string> tokens;
    if (str.empty()) {
        return tokens;
    }

    for (std::string::size_type startPos = 0; ; ) {
        std::string::size_type endPos = str.find_first_of(delim, startPos);
        tokens.push_back(str.substr(startPos, endPos));
        if (endPos == std::string::npos) {
            break;
        }
        startPos = endPos + 1;
    }
    return tokens;
}

} // namespace utils
