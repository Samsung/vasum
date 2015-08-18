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
 * @author  Jan Olszak (j.olszak@samsung.com)
 * @brief   IPC implementation for related processes
 */

#include "utils/channel.hpp"
#include "utils/exception.hpp"

#include "logger/logger.hpp"

#include <sys/socket.h>

namespace {
const int LEFT = 0;
const int RIGHT = 1;
}

namespace utils {

Channel::Channel()
    : mSocketIndex(-1)
{
    if (::socketpair(AF_LOCAL, SOCK_STREAM | SOCK_CLOEXEC, 0, mSockets) < 0) {
        const std::string msg = "socketpair() failed: " +
                                utils::getSystemErrorMessage();
        LOGE(msg);
        throw UtilsException(msg);
    }
}

Channel::~Channel()
{
    closeSocket(LEFT);
    closeSocket(RIGHT);
}

void Channel::setLeft()
{
    mSocketIndex = LEFT;
    utils::close(mSockets[RIGHT]);
    mSockets[RIGHT] = -1;
}

void Channel::setRight()
{
    mSocketIndex = RIGHT;
    utils::close(mSockets[LEFT]);
    mSockets[LEFT] = -1;
}

void Channel::shutdown()
{
    assert(mSocketIndex != -1 && "Channel's end isn't set");
    closeSocket(mSocketIndex);
}

void Channel::closeSocket(int socketIndex)
{
    utils::shutdown(mSockets[socketIndex]);
    utils::close(mSockets[socketIndex]);
    mSockets[socketIndex] = -1;
}

} // namespace utils
