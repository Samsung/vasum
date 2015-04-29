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
 * @author Jan Olszak (j.olszak@samsung.com)
 * @brief  Definition of a class for writing and reading data from a file descriptor
 */

#include "config/config.hpp"

#include "config/fdstore.hpp"
#include "config/exception.hpp"

#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <chrono>
#include <poll.h>

namespace config {

namespace {

const int ERROR_MESSAGE_BUFFER_CAPACITY = 256;

std::string getSystemErrorMessage()
{
    char buf[ERROR_MESSAGE_BUFFER_CAPACITY];
    return strerror_r(errno, buf, sizeof(buf));
}


void waitForEvent(int fd,
                  short event,
                  const std::chrono::high_resolution_clock::time_point deadline)
{
    // Wait for the rest of the data
    struct pollfd fds[1];
    fds[0].fd = fd;
    fds[0].events = event | POLLHUP;

    for (;;) {
        std::chrono::milliseconds timeoutMS =
            std::chrono::duration_cast<std::chrono::milliseconds>(deadline - std::chrono::high_resolution_clock::now());
        if (timeoutMS.count() < 0) {
            throw ConfigException("Timeout");
        }

        int ret = ::poll(fds, 1 /*fds size*/, timeoutMS.count());

        if (ret == -1) {
            if (errno == EINTR) {
                continue;
            }
            throw ConfigException("Error in poll: " + getSystemErrorMessage());
        }

        if (ret == 0) {
            throw ConfigException("Timeout");
        }

        if (fds[0].revents & POLLHUP) {
            throw ConfigException("Peer disconnected");
        }

        // Here Comes the Sun
        break;
    }
}

} // namespace

FDStore::FDStore(int fd)
    : mFD(fd)
{
}

FDStore::FDStore(const FDStore& store)
    : mFD(store.mFD)
{
}

FDStore::~FDStore()
{
}

void FDStore::write(const void* bufferPtr, const size_t size, const unsigned int timeoutMS)
{
    std::chrono::high_resolution_clock::time_point deadline = std::chrono::high_resolution_clock::now() +
                                                              std::chrono::milliseconds(timeoutMS);

    size_t nTotal = 0;
    for (;;) {
        int n  = ::write(mFD,
                         reinterpret_cast<const char*>(bufferPtr) + nTotal,
                         size - nTotal);
        if (n >= 0) {
            nTotal += n;
            if (nTotal == size) {
                // All data is written, break loop
                break;
            }
        } else if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
            // Neglected errors
        } else {
            throw ConfigException("Error during writing: " + getSystemErrorMessage());
        }

        waitForEvent(mFD, POLLOUT, deadline);
    }
}

void FDStore::read(void* bufferPtr, const size_t size, const unsigned int timeoutMS)
{
    std::chrono::high_resolution_clock::time_point deadline = std::chrono::high_resolution_clock::now() +
                                                              std::chrono::milliseconds(timeoutMS);

    size_t nTotal = 0;
    for (;;) {
        int n  = ::read(mFD,
                        reinterpret_cast<char*>(bufferPtr) + nTotal,
                        size - nTotal);
        if (n >= 0) {
            nTotal += n;
            if (nTotal == size) {
                // All data is read, break loop
                break;
            }
            if (n == 0) {
                throw ConfigException("Peer disconnected");
            }
        } else if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
            // Neglected errors
        } else {
            throw ConfigException("Error during reading: " + getSystemErrorMessage());
        }

        waitForEvent(mFD, POLLIN, deadline);
    }
}
} // namespace config
