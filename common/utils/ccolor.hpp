/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Dariusz Michaluk <d.michaluk@samsung.com>
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
 * @author  Dariusz Michaluk (d.michaluk@samsung.com)
 * @brief   Console colors utility
 */

#ifndef COMMON_UTILS_CCOLOR_HPP
#define COMMON_UTILS_CCOLOR_HPP

#include <string>

namespace utils {

enum class Color : unsigned int {
    DEFAULT     = 0,
    BLACK       = 30,
    RED         = 31,
    GREEN       = 32,
    YELLOW      = 33,
    BLUE        = 34,
    MAGENTA     = 35,
    CYAN        = 36,
    WHITE       = 37
};

enum class Attributes : unsigned int {
    DEFAULT     = 0,
    BOLD        = 1
};

std::string getConsoleEscapeSequence(Attributes attr, Color color);

} // namespace utils

#endif // COMMON_UTILS_CCOLOR_HPP
