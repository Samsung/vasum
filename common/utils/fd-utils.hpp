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

#include <string>
#include <cstddef>
#include <sys/types.h>

namespace utils {

/**
 * Open a file.
 */
int open(const std::string &path, int flags, mode_t mode = -1);

/**
 * Close the file descriptor.
 */
void close(int fd);

/**
 * Shut down part of a full-duplex connection
 */
void shutdown(int fd);

/**
 * Operation on a special file
 */
int ioctl(int fd, unsigned long request, void *argp);

/**
 * Duplicate one file desciptor onto another
 */
int dup2(int olfFD, int newFD, bool closeOnExec = false);

/**
 * Write to a file descriptor, throw on error.
 *
 * @param fd file descriptor
 * @param bufferPtr pointer to the data buffer
 * @param size size of data to write
 * @param timeoutMS timeout in milliseconds
 */
void write(int fd, const void* bufferPtr, const size_t size, int timeoutMS = 5000);

/**
 * Read from a file descriptor, throw on error.
 *
 * @param fd file descriptor
 * @param bufferPtr pointer to the data buffer
 * @param size size of the data to read
 * @param timeoutMS timeout in milliseconds
 */
void read(int fd, void* bufferPtr, const size_t size, int timeoutMS = 5000);

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

/**
 * Send Socket via Unix Domain socket
 */
bool fdSend(int socket, int fd, const unsigned int timeoutMS = 5000);

/**
 * Receive fd via Unix Domain socket
 */
int fdRecv(int socket, const unsigned int timeoutMS = 5000);

/**
 * Set or remove CLOEXEC on a file descriptor
 */
void setCloseOnExec(int fd, bool closeOnExec);

/**
 * Set or remove NONBLOCK on a file descriptor
 */
void setNonBlocking(int fd, bool nonBlocking);

} // namespace utils

#endif // COMMON_UTILS_FD_HPP
