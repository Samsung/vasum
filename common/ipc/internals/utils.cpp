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
 * @brief   Utility functions
 */

#include "config.hpp"

#include "ipc/exception.hpp"
#include "ipc/internals/utils.hpp"
#include "logger/logger.hpp"

#include <cerrno>
#include <cstring>
#include <unistd.h>

#include <sys/resource.h>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

namespace security_containers {
namespace ipc {

void close(int fd)
{
    if (fd < 0) {
        return;
    }

    for (;;) {
        if (-1 == ::close(fd)) {
            if (errno == EINTR) {
                LOGD("Close interrupted by a signal, retrying");
                continue;
            }
            LOGE("Error in close: " << std::string(strerror(errno)));
            throw IPCException("Error in close: " + std::string(strerror(errno)));
        }
        break;
    }
}

void write(int fd, const void* bufferPtr, const size_t size)
{
    size_t nTotal = 0;
    int n;

    do {
        n  = ::write(fd,
                     reinterpret_cast<const char*>(bufferPtr) + nTotal,
                     size - nTotal);
        if (n < 0) {
            if (errno == EINTR) {
                LOGD("Write interrupted by a signal, retrying");
                continue;
            }
            LOGE("Error during writing: " + std::string(strerror(errno)));
            throw IPCException("Error during witting: " + std::string(strerror(errno)));
        }
        nTotal += n;
    } while (nTotal < size);
}

void read(int fd, void* bufferPtr, const size_t size)
{
    size_t nTotal = 0;
    int n;

    do {
        n  = ::read(fd,
                    reinterpret_cast<char*>(bufferPtr) + nTotal,
                    size - nTotal);
        if (n < 0) {
            if (errno == EINTR) {
                LOGD("Read interrupted by a signal, retrying");
                continue;
            }
            LOGE("Error during reading: " + std::string(strerror(errno)));
            throw IPCException("Error during reading: " + std::string(strerror(errno)));
        }
        nTotal += n;
    } while (nTotal < size);
}

unsigned int getMaxFDNumber()
{
    struct rlimit rlim;
    if (-1 ==  getrlimit(RLIMIT_NOFILE, &rlim)) {
        LOGE("Error during getrlimit: " + std::string(strerror(errno)));
        throw IPCException("Error during getrlimit: " + std::string(strerror(errno)));
    }
    return rlim.rlim_cur;
}

void setMaxFDNumber(unsigned int limit)
{
    struct rlimit rlim;
    rlim.rlim_cur = limit;
    rlim.rlim_max = limit;
    if (-1 ==  setrlimit(RLIMIT_NOFILE, &rlim)) {
        LOGE("Error during setrlimit: " + std::string(strerror(errno)));
        throw IPCException("Error during setrlimit: " + std::string(strerror(errno)));
    }
}

unsigned int getFDNumber()
{
    const std::string path = "/proc/self/fd/";
    return std::distance(fs::directory_iterator(path),
                         fs::directory_iterator());
}

} // namespace ipc
} // namespace security_containers

