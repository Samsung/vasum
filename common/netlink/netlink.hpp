/*
 *  Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Mateusz Malicki <m.malicki2@samsung.com>
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
 * @author  Mateusz Malicki (m.malicki2@samsung.com)
 * @brief   Netlink class declaration
 */

#ifndef COMMON_NETLINK_NETLINK_HPP
#define COMMON_NETLINK_NETLINK_HPP

#include <memory>
#include <vector>

namespace vasum {
namespace netlink {

/**
 * Netlink class is responsible for communicating
 * with kernel through netlink interface
 */
class Netlink {
public:
    Netlink();
    ~Netlink();

    Netlink(const Netlink& other) = delete;
    Netlink& operator=(const Netlink& other) = delete;

    /**
     * Open connnection
     *
     * @param netNsPid pid which defines net namespace
     */
    void open(int netNsPid = 0);

    /**
     * Close connection
     */
    void close();

    /**
     * Send message
     *
     * It is not thread safe and even you shouldn't call this function on
     * different instances at the same time
     *
     * @param nlmsg pointer to message
     * @return sequence number
     */
    unsigned int send(const void* nlmsg);

    /**
     * Receive message
     *
     * It is not thread safe and even you shouldn't call this function on
     * different instances at the same time
     *
     * @param nlmsgSeq sequence number
     * @return received data
     */
    std::unique_ptr<std::vector<char>> rcv(unsigned int nlmsgSeq);
private:
    int mFd;
};

} // namesapce netlink
} // namespace vasum

#endif /* COMMON_NETLINK_NETLINK_HPP */
