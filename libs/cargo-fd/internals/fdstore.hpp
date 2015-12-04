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
 * @brief  Declaration of a class for writing and reading data from a file descriptor
 */

#ifndef CARGO_FD_INTERNALS_FDSTORE_HPP
#define CARGO_FD_INTERNALS_FDSTORE_HPP

#include <cstddef>

namespace {
const unsigned int maxTimeout = 5000;
} // namespace

namespace cargo {

namespace internals {

class FDStore {

public:
    /**
     * Constructor. One can pass any kind of file descriptor.
     * Serialization is NOT written for network purposes,
     * rather local communication.
     *
     * @param fd file descriptor
     */
    FDStore(int fd = -1);
    FDStore(const FDStore& store);
    ~FDStore();

    /**
     * Write data using the file descriptor
     *
     * @param bufferPtr buffer with the data
     * @param size size of the buffer
     * @param timeoutMS timeout in milliseconds
     */
    void write(const void* bufferPtr, const size_t size, const unsigned int timeoutMS = maxTimeout);

    /**
     * Reads a value of the given type.
     *
     * @param bufferPtr buffer with the data
     * @param size size of the buffer
     * @param timeoutMS timeout in milliseconds
     */
    void read(void* bufferPtr, const size_t size, const unsigned int timeoutMS = maxTimeout);

    void sendFD(int fd, const unsigned int timeoutMS = maxTimeout);

    int receiveFD(const unsigned int timeoutMS = maxTimeout);

private:
    int mFD;
};

} // namespace internals

} // namespace cargo

#endif // CARGO_FD_INTERNALS_FDSTORE_HPP


