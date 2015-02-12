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
 * @brief   Netlink class definition
 */

#include "config.hpp"
#include "netlink.hpp"
#include "utils.hpp"
#include "base-exception.hpp"

#include <logger/logger.hpp>
#include <cassert>
#include <algorithm>
#include <sys/socket.h>
#include <unistd.h>
#include <linux/netlink.h>

namespace vasum {

namespace {

template<class T>
T make_clean()
{
    static_assert(std::is_pod<T>::value, "make_clean require trivial and standard-layout");
    T value;
    std::fill_n(reinterpret_cast<char*>(&value), sizeof(value), 0);
    return value;
}

} // namespace

Netlink::Netlink() : mFd(-1)
{
}

Netlink::~Netlink()
{
    close();
}

void Netlink::open()
{
    assert(mFd == -1);
    mFd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    if (mFd == -1) {
        LOGE("Can't open socket (" << getSystemErrorMessage() << ")");
        throw VasumException("Can't open netlink connection");
    }

    sockaddr_nl local = make_clean<sockaddr_nl>();
    local.nl_family = AF_NETLINK;

    if (bind(mFd, (struct sockaddr *)&local, sizeof(local)) < 0) {
        int err = errno;
        close();
        LOGE("Can't bind to socket (" << getSystemErrorMessage(err) << ")");
        throw VasumException("Can't set up netlink connection");
    }
}

void Netlink::close()
{
    if (mFd != -1) {
        ::close(mFd);
        mFd = -1;
    }
}

void Netlink::send(const nlmsghdr *nlmsg)
{
    msghdr msg = make_clean<msghdr>();
    sockaddr_nl nladdr = make_clean<sockaddr_nl>();
    iovec iov = make_clean<iovec>();

    iov.iov_base = (void *)nlmsg;
    iov.iov_len = nlmsg->nlmsg_len;
    msg.msg_name = &nladdr;
    msg.msg_namelen = sizeof(nladdr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    nladdr.nl_family = AF_NETLINK;

    int ret = sendmsg(mFd, &msg, 0);
    if (ret < 0) {
        LOGE("Can't send message (" << getSystemErrorMessage() << ")");
        throw VasumException("Can't send netlink message");
    }
}

int Netlink::rcv(nlmsghdr *answer)
{
    //TODO: Handle too small buffer situation (buffer resizing)
    msghdr msg = make_clean<msghdr>();
    sockaddr_nl nladdr = make_clean<sockaddr_nl>();
    iovec iov = make_clean<iovec>();

    iov.iov_base = answer;
    iov.iov_len = answer->nlmsg_len;
    msg.msg_name = &nladdr;
    msg.msg_namelen = sizeof(nladdr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    nladdr.nl_family = AF_NETLINK;

    int ret = recvmsg(mFd, &msg, 0);
    if (ret < 0) {
        LOGE("Can't receive message (" + getSystemErrorMessage() + ")");
        throw VasumException("Can't receive netlink message");
    }
    if (ret == 0) {
        LOGE("Peer has performed an orderly shutdown");
        throw VasumException("Can't receive netlink message");
    }
    if (msg.msg_flags & MSG_TRUNC) {
        LOGE("Can't receive message (" + getSystemErrorMessage(EMSGSIZE) + ")");
        throw VasumException("Can't receive netlink message");
    }
    if (msg.msg_flags & MSG_ERRQUEUE) {
        LOGE("No data was received but an extended error");
        throw VasumException("Can't receive netlink message");
    }
    if (msg.msg_flags & MSG_OOB) {
        LOGE("Internal error (expedited or out-of-band data were received)");
        throw VasumException("Can't receive netlink message");
    }
    if (msg.msg_flags & (MSG_EOR | MSG_CTRUNC)) {
        assert(!"This should not happen!");
        LOGE("Internal error (" << std::to_string(msg.msg_flags) << ")");
        throw VasumException("Internal error while recaiving netlink message");
    }

    return ret;
}

} //namespace vasum
