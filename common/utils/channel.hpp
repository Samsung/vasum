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

#ifndef COMMON_UTILS_CHANNEL_HPP
#define COMMON_UTILS_CHANNEL_HPP

#include "utils/fd-utils.hpp"

#include <array>
#include <cassert>

namespace utils {

/**
 * Channel is implemented with a pair of anonymous sockets
 */
class Channel {
public:
    explicit Channel(const bool closeOnExec = true);
    explicit Channel(const int fd);
    ~Channel();

    Channel(const Channel&) = delete;
    Channel& operator=(const Channel&) = delete;

    /**
     * Use the "left" end of the channel
     * Closes the "right" end
     */
    void setLeft();

    /**
    * Use the "right" end of the channel
    * Closes the "left" end
    */
    void setRight();

    /**
     * Gracefully shutdown the used end of the channel
     */
    void shutdown();

    /**
     * Send the data to the other end of the channel
     *
     * @param data data to send
     */
    template<typename Data>
    void write(const Data& data);

    /**
     * Receive data of a given type (size)
     */
    template<typename Data>
    Data read();

    /**
     * Get an active file descriptor
     */
    int getFD();

    /**
     * Gen the left file descriptor
     */
    int getLeftFD();

    /**
     * Gen the right file descriptor
     */
    int getRightFD();

    /**
     * Sets close on exec on an active fd to either true or false
     */
    void setCloseOnExec(const bool closeOnExec);

private:

    void closeSocket(int socketIndex);

    int mSocketIndex;
    std::array<int, 2> mSockets;
};

template<typename Data>
void Channel::write(const Data& data)
{
    assert(mSocketIndex != -1 && "Channel's end isn't set");

    utils::write(mSockets[mSocketIndex], &data, sizeof(Data));
}

template<typename Data>
Data Channel::read()
{
    assert(mSocketIndex != -1 && "Channel's end isn't set");

    Data data;
    utils::read(mSockets[mSocketIndex], &data, sizeof(Data));
    return data;
}

} // namespace utils

#endif // COMMON_UTILS_CHANNEL_HPP
