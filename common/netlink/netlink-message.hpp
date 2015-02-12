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
 * @brief   Netlink message class declaration
 */

#ifndef COMMON_NETLINK_NETLINK_MESSAGE_HPP
#define COMMON_NETLINK_NETLINK_MESSAGE_HPP

#include <string>
#include <vector>
#include <stack>
#include <type_traits>
#include <cstdlib>
#include <linux/netlink.h>

namespace vasum {
namespace netlink {

/**
 *  NetlinkMessage is used to creatie a netlink messages
 */
class NetlinkMessage {
public:
    /**
     * Create netlink message
     *
     * @param type rtnetlink message type (see man 7 rtnetlink)
     * @param flags nlmsg flags (see man 7 netlink)
     */
    NetlinkMessage(std::uint16_t type, std::uint16_t flags);

    /**
     * Add nested atribute type
     *
     * All future attributes will be nested in this attribute (till call to endNested)
     *
     * @param ifla attribute name
     */
    NetlinkMessage& beginNested(int ifla);

    /**
     * End nested atribute
     */
    NetlinkMessage& endNested();

    ///@{
    /*Add sample attribute */
    NetlinkMessage& put(int ifla, const std::string& value);
    template<class T>
    NetlinkMessage& put(int ifla, const T& value);
    ///@}

    /**
     * Add raw data
     *
     * Add raw data to end of netlink message
     */
    template<class T>
    NetlinkMessage& put(const T& value);

    /**
     * Send netlink message
     *
     * It is not thread safe
     */
    friend void send(const NetlinkMessage& msg);
private:
    std::vector<char> mNlmsg;
    std::stack<void*> mNested;

    NetlinkMessage& put(int ifla, const void* data, int len);
    NetlinkMessage& put(const void* data, int len);
    nlmsghdr& hdr();
    const nlmsghdr& hdr() const;
    void setMinCapacity(unsigned int size);


};

template<class T>
NetlinkMessage& NetlinkMessage::put(int ifla, const T& value)
{
    static_assert(std::is_pod<T>::value, "Require trivial and standard-layout");
    return put(ifla, &value, sizeof(value));
}

template<class T>
NetlinkMessage& NetlinkMessage::put(const T& value)
{
    static_assert(std::is_pod<T>::value, "Require trivial and standard-layout structure");
    return put(&value, sizeof(value));
}

} // namespace netlink
} // namespace vasum

#endif // COMMON_NETLINK_NETLINK_MESSAGE_HPP
