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

#include "config.hpp"

#include "utils/ccolor.hpp"

#include <stdio.h>

namespace utils {

std::string getConsoleEscapeSequence(Attributes attr, Color color)
{
    char command[10];

    // Command is the control command to the terminal
    snprintf(command, sizeof(command), "%c[%u;%um", 0x1B, (unsigned int)attr, (unsigned int)color);
    return std::string(command);
}

} // namespace utils
