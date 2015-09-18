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
 * @brief   Linux socket wrapper
 */

#include "config.hpp"

#include "utils/eventfd.hpp"
#include "utils/exception.hpp"
#include "utils/fd-utils.hpp"
#include "logger/logger.hpp"

#include <sys/eventfd.h>
#include <cerrno>
#include <cstring>
#include <cstdint>

namespace utils {

EventFD::EventFD()
{
    mFD = ::eventfd(0, EFD_SEMAPHORE | EFD_CLOEXEC);
    if (mFD == -1) {
        const std::string msg = "Error in eventfd: " + getSystemErrorMessage();
        LOGE(msg);
        throw EventFDException(msg);
    }
}

EventFD::~EventFD()
{
    utils::close(mFD);
}

int EventFD::getFD() const
{
    return mFD;
}

void EventFD::send()
{
    const std::uint64_t toSend = 1;
    utils::write(mFD, &toSend, sizeof(toSend));
}

void EventFD::receive()
{
    std::uint64_t readBuffer;
    utils::read(mFD, &readBuffer, sizeof(readBuffer));
}


} // namespace utils
