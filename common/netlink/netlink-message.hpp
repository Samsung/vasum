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

#include <limits>
#include <memory>
#include <string>
#include <vector>
#include <stack>
#include <type_traits>
#include <cstdlib>
#include <sys/socket.h>
#include <linux/netlink.h>

//FIXME remove from namespace vasum
namespace vasum {
namespace netlink {

class NetlinkResponse;
class NetlinkMessage;

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
     * @param msg Netlink message
     * @param pid Process id which describes network namespace
     */
    friend NetlinkResponse send(const NetlinkMessage& msg, int pid);
private:
    std::vector<char> mNlmsg;
    std::stack<int> mNested;

    NetlinkMessage& put(int ifla, const void* data, int len);
    NetlinkMessage& put(const void* data, int len);
    nlmsghdr& hdr();
    const nlmsghdr& hdr() const;
    void setMinCapacity(unsigned int size);
};

/**
 *  NetlinkResponse is used to read netlink messages
 */
class NetlinkResponse {
public:
    /**
     * Check if theres is next message in netlink response
     */
    bool hasMessage() const;

    /**
     * Fetch next message
     */
    void fetchNextMessage();

    /**
     * Get message type
     */
    int getMessageType() const;

    /**
     * Check if there is any attribute in message
     */
    bool hasAttribute() const;

    /**
     * Check if current attribute is nested
     */
    bool isNestedAttribute() const;

    /**
     * Skip attribute
     */
    void skipAttribute();

    /**
     * Start reading nested attribute
     */
    NetlinkResponse& openNested(int ifla);

    /**
     * End reading nested attribute
     */
    NetlinkResponse& closeNested();

    ///@{
    /**
     * Fetch attribute
     */
    NetlinkResponse& fetch(int ifla, std::string& value, unsigned maxLen = std::numeric_limits<unsigned>::max());
    template<class T>
    NetlinkResponse& fetch(int ifla, T& value);
    ///@}

    /**
     * Get attributie type
     **/
    int getAttributeType() const;

    /**
     * Get attributie length
     **/
    int getAttributeLength() const;

    /**
     * Fetch data of type T
     */
    template<class T>
    NetlinkResponse& fetch(T& value);

    /**
     * Skip data of type T
     */
    template<class T>
    NetlinkResponse& skip();

    /**
     * Send netlink message
     *
     * It is not thread safe
     * @param msg Netlink message
     * @param pid Process id which describes network namespace
     */
    friend NetlinkResponse send(const NetlinkMessage& msg, int pid);
private:
    NetlinkResponse(std::unique_ptr<std::vector<char>>&& message);

    std::unique_ptr<std::vector<char>> mNlmsg;
    std::stack<int> mNested;
    nlmsghdr* mNlmsgHdr;
    int mPosition;

    const char* get(int ifla, int iflasize) const;
    const char* get(int size = 0) const;
    NetlinkResponse& fetch(int ifla, char* data, int len);
    NetlinkResponse& fetch(char* data, int len);
    NetlinkResponse& seek(int len);
    int size() const;
    int getHdrPosition() const;
};

/**
 * Send netlink message
 *
 * It is not thread safe
 */
NetlinkResponse send(const NetlinkMessage& msg);

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

template<class T>
NetlinkResponse& NetlinkResponse::fetch(int ifla, T& value)
{
    static_assert(std::is_pod<T>::value, "Require trivial and standard-layout");
    return fetch(ifla, reinterpret_cast<char*>(&value), sizeof(value));
}

template<class T>
NetlinkResponse& NetlinkResponse::fetch(T& value)
{
    static_assert(std::is_pod<T>::value, "Require trivial and standard-layout structure");
    return fetch(reinterpret_cast<char*>(&value), sizeof(value));
}

template<class T>
NetlinkResponse& NetlinkResponse::skip()
{
    static_assert(std::is_pod<T>::value, "Require trivial and standard-layout structure");
    return seek(sizeof(T));
}

} // namespace netlink
} // namespace vasum

#endif // COMMON_NETLINK_NETLINK_MESSAGE_HPP
