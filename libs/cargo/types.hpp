/*
 *  Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Jan Olszak <j.olszak@samsung.com>
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
 * @author Jan Olszak (j.olszak@samsung.com)
 * @brief  Types declarations
 */

#ifndef CARGO_TYPES_HPP
#define CARGO_TYPES_HPP

namespace cargo {

/**
 * Whenever possible, this type will be serialized using Linux file descriptor passing.
 */
struct FileDescriptor {
    int value;
    FileDescriptor(int fd = -1): value(fd) {}
    FileDescriptor& operator=(const int fd) {
        value = fd;
        return *this;
    }
};

} // cargo

#endif //CARGO_TYPES_HPP
