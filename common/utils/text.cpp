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
 * @brief   Text related utility
 */

#include "config.hpp"

#include "utils/text.hpp"

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

std::vector<std::string> split(const std::string& str, const std::string& delim)
{
    std::vector<std::string> tokens;
    if (str.empty()) {
        return tokens;
    }

    for (std::string::size_type startPos = 0; ; ) {
        std::string::size_type endPos = str.find_first_of(delim, startPos);

        if (endPos == std::string::npos) {
            tokens.push_back(str.substr(startPos, endPos));
            break;
        }
        tokens.push_back(str.substr(startPos, endPos - startPos));

        startPos = endPos + 1;
    }
    return tokens;
}

} // namespace utils
