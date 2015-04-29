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
 * @brief   Signal related functions
 */

#include "utils/signal.hpp"
#include "utils/exception.hpp"
#include "logger/logger.hpp"

#include <string>
#include <cerrno>
#include <cstring>
#include <csignal>

namespace vasum {
namespace utils {

void signalBlock(const int signalToBlock)
{
    ::sigset_t set;
    if (-1 == ::sigemptyset(&set)) {
        const std::string msg = getSystemErrorMessage();
        LOGE("Error in sigemptyset: " << msg);
        throw UtilsException("Error in sigemptyset: " + msg);
    }

    if (-1 ==::sigaddset(&set, signalToBlock)) {
        const std::string msg = getSystemErrorMessage();
        LOGE("Error in sigaddset: " << msg);
        throw UtilsException("Error in sigaddset: " + msg);
    }

    int ret = ::pthread_sigmask(SIG_BLOCK, &set, nullptr /*&oldSet*/);
    if (ret != 0) {
        LOGE("Error in pthread_sigmask: " << std::to_string(ret));
        throw UtilsException("Error in pthread_sigmask: " + std::to_string(ret));
    }
}

} // namespace utils
} // namespace vasum





