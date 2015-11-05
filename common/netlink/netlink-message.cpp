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
#include "logger/logger.hpp"

#include <algorithm>
#include <memory>
#include <cstring>
#include <atomic>
#include <cassert>

#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <sys/socket.h>
#include <unistd.h>

namespace {

inline const rtattr* asAttr(const void* data) { return reinterpret_cast<const rtattr*>(data); }
inline const nlmsghdr* asHdr(const void* data) { return reinterpret_cast<const nlmsghdr*>(data); }
inline rtattr* asAttr(void* data) { return reinterpret_cast<rtattr*>(data); }
inline nlmsghdr* asHdr(void* data) { return reinterpret_cast<nlmsghdr*>(data); }
inline char* NLMSG_TAIL(nlmsghdr* nmsg)
{
    return reinterpret_cast<char*>(nmsg) + NLMSG_ALIGN(nmsg->nlmsg_len);
}

} // namespace

namespace vasum {
namespace netlink {

NetlinkResponse send(const NetlinkMessage& msg)
{
    return send(msg, 0);
}

NetlinkMessage::NetlinkMessage(uint16_t type, uint16_t flags)
{
    static std::atomic<uint32_t> seq(0);
    mNlmsg.resize(NLMSG_HDRLEN, 0);
    hdr().nlmsg_len = NLMSG_HDRLEN;
    hdr().nlmsg_flags = flags | NLM_F_ACK;
    hdr().nlmsg_type = type;
    hdr().nlmsg_seq = ++seq;
    hdr().nlmsg_pid = getpid();
}

NetlinkMessage& NetlinkMessage::beginNested(int ifla)
{
    auto offset = std::distance(reinterpret_cast<char*>(&hdr()), NLMSG_TAIL(&hdr()));
    put(ifla, NULL, 0);
    mNested.push(offset);
    return *this;
}

NetlinkMessage& NetlinkMessage::endNested()
{
    assert(!mNested.empty());
    rtattr* nest = asAttr(reinterpret_cast<char*>(&hdr()) +  mNested.top());
    nest->rta_len = std::distance(reinterpret_cast<char*>(nest), NLMSG_TAIL(&hdr()));
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
    rta = asAttr(NLMSG_TAIL(&hdr()));
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
    return *asHdr(mNlmsg.data());
}

const nlmsghdr& NetlinkMessage::hdr() const {
    return *asHdr(mNlmsg.data());
}

void NetlinkMessage::setMinCapacity(unsigned int size)
{
    if (mNlmsg.size() < size) {
        mNlmsg.resize(size, 0);
    }
}

NetlinkResponse::NetlinkResponse(std::unique_ptr<std::vector<char>>&& message)
    : mNlmsg(std::move(message))
    , mNlmsgHdr(asHdr(mNlmsg.get()->data()))
    , mPosition(NLMSG_HDRLEN)
{
}

bool NetlinkResponse::hasMessage() const
{
    unsigned int tail = size() - getHdrPosition();
    bool hasHeader = NLMSG_OK(mNlmsgHdr, tail);
    if (!hasHeader) {
        return false;
    }
    //Check if isn't ACK message
    return NLMSG_PAYLOAD(mNlmsgHdr,0) > sizeof(uint32_t);
}

int NetlinkResponse::getMessageType() const
{
    return mNlmsgHdr->nlmsg_type;
}

void NetlinkResponse::fetchNextMessage()
{
    if (mNlmsgHdr->nlmsg_type == NLMSG_DONE) {
        throw VasumException("There is no next message");
    }
    int tail = size() - mPosition;
    mNlmsgHdr = NLMSG_NEXT(mNlmsgHdr, tail);
    mPosition = getHdrPosition() + NLMSG_HDRLEN;
}

bool NetlinkResponse::hasAttribute() const
{
    assert(mPosition >= getHdrPosition());
    int tail = mNlmsgHdr->nlmsg_len - (mPosition - getHdrPosition());
    return RTA_OK(asAttr(get(0)), tail);
}

bool NetlinkResponse::isNestedAttribute() const
{
    return asAttr(get(RTA_LENGTH(0)))->rta_len == RTA_LENGTH(0);
}

void NetlinkResponse::skipAttribute()
{
    const rtattr *rta = asAttr(get(RTA_LENGTH(0)));
    if (size() < mPosition + RTA_ALIGN(rta->rta_len)) {
        LOGE("Skipping out of buffer:"
                << " to: " << mPosition + RTA_ALIGN(rta->rta_len)
                << ", buf size: " << size());
        throw VasumException("Skipping out of buffer");
    }
    seek(RTA_ALIGN(rta->rta_len));
}

NetlinkResponse& NetlinkResponse::openNested(int ifla)
{
    const rtattr *rta = asAttr(get(RTA_LENGTH(0)));
    if (rta->rta_type != ifla) {
        const std::string msg = "Wrong attribute type, expected: " + std::to_string(ifla) + ", got: " + std::to_string(rta->rta_type);
        LOGE(msg);
        throw VasumException(msg);
    }
    int pos = mPosition;
    seek(RTA_LENGTH(0));
    mNested.push(pos);
    return *this;
}

NetlinkResponse& NetlinkResponse::closeNested()
{
    assert(!mNested.empty());
    int pos = mNested.top();
    const rtattr *rta = asAttr(mNlmsg->data() + pos);
    if (rta->rta_len != mPosition - pos) {
        LOGE("There is no nested attribute end. Did you read all attributes (read: "
                << mPosition -  pos << ", length: " << rta->rta_len);
        throw VasumException("There is no nested attribute end");
    }
    mNested.pop();
    mPosition = pos;
    return *this;
}

NetlinkResponse& NetlinkResponse::fetch(int ifla, std::string& value, unsigned len)
{
    const rtattr *rta = asAttr(get(RTA_LENGTH(0)));
    if (len > RTA_PAYLOAD(rta)) {
        len = RTA_PAYLOAD(rta);
    }
    value = std::string(get(ifla, -1), len);
    skipAttribute();
    return *this;
}

const char* NetlinkResponse::get(int ifla, int len) const
{
    const rtattr *rta = asAttr(get(RTA_LENGTH(len < 0 ? 0 : len)));
    if (rta->rta_type != ifla) {
        const std::string msg = "Wrong attribute type, expected: " + std::to_string(ifla) +
                                ", got: " + std::to_string(rta->rta_type);
        LOGE(msg);
        throw VasumException(msg);
    }
    if (len >= 0 && rta->rta_len != RTA_LENGTH(len)) {
        const std::string msg = "Wrong attribute " + std::to_string(ifla) +
                                " length, expected: " + std::to_string(rta->rta_len) +
                                ", got: " + std::to_string(RTA_LENGTH(len));
        LOGE(msg);
        throw VasumException(msg);
    }
    return reinterpret_cast<const char*>(RTA_DATA(get(rta->rta_len)));
}

const char* NetlinkResponse::get(int len) const
{
    if (size() < mPosition + len) {
        LOGE("Read out of buffer:"
                << " from: " << mPosition + len
                << ", buf size: " << size());
        throw VasumException("Read out of buffer");
    }
    return mNlmsg->data() + mPosition;
}

NetlinkResponse& NetlinkResponse::fetch(int ifla, char* data, int len)
{
    std::copy_n(get(ifla, len), len, data);
    skipAttribute();
    return *this;
}

NetlinkResponse& NetlinkResponse::fetch(char* data, int len)
{
    std::copy_n(get(len), len, data);
    seek(len);
    return *this;
}

int NetlinkResponse::getAttributeType() const
{
    return asAttr(get(RTA_LENGTH(0)))->rta_type;
}

int NetlinkResponse::getAttributeLength() const
{
    return RTA_PAYLOAD(asAttr(get(RTA_LENGTH(0))));
}

NetlinkResponse& NetlinkResponse::seek(int len)
{
    if (size() < mPosition + len) {
        throw VasumException("Skipping out of buffer");
    }
    mPosition += len;
    return *this;
}

int NetlinkResponse::size() const
{
    return mNlmsg->size();
}

inline int NetlinkResponse::getHdrPosition() const
{
    return std::distance(reinterpret_cast<const char*>(mNlmsg->data()),
                         reinterpret_cast<const char*>(mNlmsgHdr));
}

NetlinkResponse send(const NetlinkMessage& msg, int pid)
{
    const auto &hdr = msg.hdr();
    assert(hdr.nlmsg_flags & NLM_F_ACK);

    std::unique_ptr<std::vector<char>> data;
    Netlink nl;
    nl.open(pid);
    try {
        nl.send(&hdr);
        data = nl.rcv(hdr.nlmsg_seq);
    } catch (const std::exception& ex) {
        LOGE("Sending failed (" << ex.what() << "), pid=" + std::to_string(pid));
        nl.close();
        throw;
    }
    nl.close();
    return NetlinkResponse(std::move(data));
}

} // namespace netlink
} // namespace vasum
