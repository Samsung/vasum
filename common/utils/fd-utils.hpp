/*
*  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
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
 * @author  Jan Olszak (j.olszak@samsung.com)
 * @brief   File descriptor utility functions
 */

#ifndef COMMON_UTILS_FD_HPP
#define COMMON_UTILS_FD_HPP

#include <cstddef>

namespace vasum {
namespace utils {

/**
 * Close the file descriptor.
 */
void close(int fd);

/**
 * Write to a file descriptor, throw on error.
 *
 * @param fd file descriptor
 * @param bufferPtr pointer to the data buffer
 * @param size size of data to write
 * @param timeoutMS timeout in milliseconds
 */
void write(int fd, const void* bufferPtr, const size_t size, int timeoutMS = 500);

/**
 * Read from a file descriptor, throw on error.
 *
 * @param fd file descriptor
 * @param bufferPtr pointer to the data buffer
 * @param size size of the data to read
 * @param timeoutMS timeout in milliseconds
 */
void read(int fd, void* bufferPtr, const size_t size, int timeoutMS = 500);

/**
 * @return the max number of file descriptors for this process.
 */
unsigned int getMaxFDNumber();

/**
 * Set the software and hardware limit of file descriptors for this process.
 *
 * @param limit limit of file descriptors
 */
void setMaxFDNumber(unsigned int limit);

/**
 * @return number of opened file descriptors by this process
 */
unsigned int getFDNumber();

} // namespace utils
} // namespace vasum

#endif // COMMON_UTILS_FD_HPP
