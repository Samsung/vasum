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
 * @brief   Netlink message class definition
 */

#include "config.hpp"
#include "netlink-message.hpp"
#include "netlink.hpp"
#include "base-exception.hpp"

#include <logger/logger.hpp>

#include <memory>
#include <cstring>
#include <cassert>

#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <sys/socket.h>
#include <unistd.h>

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

namespace {

const int NLMSG_GOOD_SIZE = 2*PAGE_SIZE;
inline rtattr* NLMSG_TAIL(nlmsghdr* nmsg)
{
    return reinterpret_cast<rtattr*>(reinterpret_cast<char*>(nmsg) + NLMSG_ALIGN(nmsg->nlmsg_len));
}

} // namespace

namespace vasum {
namespace netlink {

NetlinkMessage::NetlinkMessage(uint16_t type, uint16_t flags)
{
    static uint32_t seq = 0;
    mNlmsg.resize(NLMSG_GOOD_SIZE, 0);
    hdr().nlmsg_len = NLMSG_HDRLEN;
    hdr().nlmsg_flags = flags | NLM_F_ACK;
    hdr().nlmsg_type = type;
    hdr().nlmsg_seq = ++seq;
    hdr().nlmsg_pid = getpid();
}

NetlinkMessage& NetlinkMessage::beginNested(int ifla)
{
    struct rtattr *nest = NLMSG_TAIL(&hdr());
    put(ifla, NULL, 0);
    mNested.push(nest);
    return *this;
}

NetlinkMessage& NetlinkMessage::endNested()
{
    assert(!mNested.empty());
    rtattr *nest = reinterpret_cast<rtattr*>(mNested.top());
    nest->rta_len = std::distance(reinterpret_cast<char*>(nest),
                                  reinterpret_cast<char*>(NLMSG_TAIL(&hdr())));
    mNested.pop();
    return *this;
}

NetlinkMessage& NetlinkMessage::put(int ifla, const std::string& value)
{
    return put(ifla, value.c_str(), value.size() + 1);
}

NetlinkMessage& NetlinkMessage::put(int ifla, const void* data, int len)
{
    struct rtattr *rta;
    size_t rtalen = RTA_LENGTH(len);
    int newLen = NLMSG_ALIGN(hdr().nlmsg_len) + RTA_ALIGN(rtalen);

    setMinCapacity(newLen);
    rta = NLMSG_TAIL(&hdr());
    rta->rta_type = ifla;
    rta->rta_len = rtalen;
    memcpy(RTA_DATA(rta), data, len);
    hdr().nlmsg_len = newLen;
    return *this;
}

NetlinkMessage& NetlinkMessage::put(const void* data, int len)
{
    setMinCapacity(hdr().nlmsg_len + len);
    memcpy((reinterpret_cast<char*>(&hdr()) + hdr().nlmsg_len), data, len);
    hdr().nlmsg_len += len;
    return *this;
}

nlmsghdr& NetlinkMessage::hdr() {
    return *reinterpret_cast<nlmsghdr*>(mNlmsg.data());
}

const nlmsghdr& NetlinkMessage::hdr() const {
    return *reinterpret_cast<const nlmsghdr*>(mNlmsg.data());
}

void NetlinkMessage::setMinCapacity(unsigned int size)
{
    if (mNlmsg.size() < size) {
        mNlmsg.resize(size, 0);
    }
}

void send(const NetlinkMessage& msg)
{
    //TODO: Handle messages with responses
    assert(msg.hdr().nlmsg_flags & NLM_F_ACK);

    const int answerLen = NLMSG_ALIGN(msg.hdr().nlmsg_len + sizeof(nlmsgerr));
    std::unique_ptr<char[]> answerBuff(new char[answerLen]);
    nlmsghdr* answer = reinterpret_cast<nlmsghdr*>(answerBuff.get());
    answer->nlmsg_len =  answerLen;

    Netlink nl;
    nl.open();
    try {
        nl.send(&msg.hdr());
        //Receive ACK Netlink Message
        do {
            nl.rcv(answer);
        } while (answer->nlmsg_type == NLMSG_NOOP);
    } catch (const std::exception& ex) {
        LOGE("Sending failed (" << ex.what() << ")");
        nl.close();
        throw;
    }
    nl.close();
    if (answer->nlmsg_type != NLMSG_ERROR) {
        // It is not NACK/ACK message
        throw VasumException("Sending failed ( unrecognized message type )");
    }
    nlmsgerr *err = reinterpret_cast<nlmsgerr*>(NLMSG_DATA(answer));
    if (answer->nlmsg_seq != msg.hdr().nlmsg_seq) {
        throw VasumException("Sending failed ( answer message was mismatched )");
    }
    if (err->error) {
        throw VasumException("Sending failed (" + getSystemErrorMessage(-err->error) + ")");
    }
}

} // namespace netlink
} // namespace vasum
