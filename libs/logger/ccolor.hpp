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
 * @brief   Console color for StderrBackend logger
 */

#ifndef COMMON_LOGGER_CCOLOR_HPP
#define COMMON_LOGGER_CCOLOR_HPP

#include <string>

namespace logger {

enum class Color : unsigned int {
    DEFAULT     = 0,
    BLACK       = 90,
    RED         = 91,
    GREEN       = 92,
    YELLOW      = 93,
    BLUE        = 94,
    MAGENTA     = 95,
    CYAN        = 96,
    WHITE       = 97
};

enum class Attributes : unsigned int {
    DEFAULT     = 0,
    BOLD        = 1
};

std::string getConsoleEscapeSequence(Attributes attr, Color color);

} // namespace logger

#endif // COMMON_LOGGER_CCOLOR_HPP
