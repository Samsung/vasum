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
#include "netlink/netlink.hpp"
#include "utils/exception.hpp"
#include "utils/make-clean.hpp"
#include "utils/environment.hpp"
#include "base-exception.hpp"
#include "logger/logger.hpp"

#include <cassert>
#include <algorithm>
#include <sys/socket.h>
#include <unistd.h>
#include <linux/netlink.h>

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

using namespace utils;
using namespace vasum;

namespace {

const int NLMSG_RCV_GOOD_SIZE = 2*PAGE_SIZE;

int vsm_recvmsg(int fd, struct msghdr *msg, int flags)
{
    int ret = recvmsg(fd, msg, flags);
    if (ret < 0) {
        LOGE("Can't receive message: " + getSystemErrorMessage());
    } else if (ret == 0 && msg->msg_iov && msg->msg_iov->iov_len > 0) {
        LOGE("Peer has performed an orderly shutdown");
    } else if (msg->msg_flags & MSG_TRUNC) {
        LOGE("Can't receive message: " + getSystemErrorMessage(EMSGSIZE));
    } else if (msg->msg_flags & MSG_ERRQUEUE) {
        LOGE("No data was received but an extended error");
    } else if (msg->msg_flags & MSG_OOB) {
        LOGE("Internal error (expedited or out-of-band data were received)");
    } else if (msg->msg_flags & MSG_CTRUNC) {
        LOGE("Some control data were discarded");
    } else if (msg->msg_flags & MSG_EOR) {
        LOGE("End-of-record");
    } else {
        // All ok
        return ret;
    }
    throw VasumException("Can't receive netlink message");
}

void vsm_sendmsg(int fd, const struct msghdr *msg, int flags)
{
    int ret = sendmsg(fd, msg, flags);
    if (ret < 0) {
        LOGE("Can't send message: " << getSystemErrorMessage());
        throw VasumException("Can't send netlink message");
    }
}

} // namespace

namespace vasum {
namespace netlink {

Netlink::Netlink() : mFd(-1)
{
}

Netlink::~Netlink()
{
    close();
}

void Netlink::open(int netNsPid)
{
    auto fdFactory = []{ return socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE); };

    assert(mFd == -1);
    if (netNsPid == 0 || netNsPid == getpid()) {
        mFd = fdFactory();
        if (mFd == -1) {
            LOGE("Can't open socket: " << getSystemErrorMessage());
        }
    } else {
        mFd = utils::passNamespacedFd(netNsPid, CLONE_NEWNET, fdFactory);
    }
    if (mFd == -1) {
        throw VasumException("Can't open netlink connection (zone not running)");
    }

    sockaddr_nl local = utils::make_clean<sockaddr_nl>();
    local.nl_family = AF_NETLINK;

    if (bind(mFd, (struct sockaddr *)&local, sizeof(local)) < 0) {
        int err = errno;
        close();
        LOGE("Can't bind to socket: " << getSystemErrorMessage(err));
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

unsigned int Netlink::send(const void *nlmsg)
{
    msghdr msg = utils::make_clean<msghdr>();
    sockaddr_nl nladdr = utils::make_clean<sockaddr_nl>();
    iovec iov = utils::make_clean<iovec>();

    iov.iov_base = const_cast<void*>(nlmsg);
    iov.iov_len = reinterpret_cast<const nlmsghdr*>(nlmsg)->nlmsg_len;
    msg.msg_name = &nladdr;
    msg.msg_namelen = sizeof(nladdr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    nladdr.nl_family = AF_NETLINK;

    vsm_sendmsg(mFd, &msg, 0);
    return reinterpret_cast<const nlmsghdr*>(nlmsg)->nlmsg_seq;
}

std::unique_ptr<std::vector<char>> Netlink::rcv(unsigned int nlmsgSeq)
{
    std::unique_ptr<std::vector<char>> buf(new std::vector<char>());

    msghdr msg = utils::make_clean<msghdr>();
    sockaddr_nl nladdr = utils::make_clean<sockaddr_nl>();
    iovec iov = utils::make_clean<iovec>();

    msg.msg_name = &nladdr;
    msg.msg_namelen = sizeof(nladdr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    nladdr.nl_family = AF_NETLINK;

    nlmsghdr* answer;
    nlmsghdr* lastOk = NULL;
    size_t offset = 0;
    do {
        buf->resize(offset + NLMSG_RCV_GOOD_SIZE);
        answer = reinterpret_cast<nlmsghdr*>(buf->data() + offset);
        iov.iov_base = answer;
        iov.iov_len = buf->size() - offset;
        unsigned int ret = vsm_recvmsg(mFd, &msg, 0);
        for (unsigned int len = ret; NLMSG_OK(answer, len); answer = NLMSG_NEXT(answer, len)) {
            lastOk = answer;
            if (answer->nlmsg_type == NLMSG_ERROR) {
                // It is NACK/ACK message
                nlmsgerr *err = reinterpret_cast<nlmsgerr*>(NLMSG_DATA(answer));
                if (answer->nlmsg_seq != nlmsgSeq) {
                    throw VasumException("Receive failed: answer message was mismatched");
                }
                if (err->error) {
                    throw VasumException("Receive failed: " + getSystemErrorMessage(-err->error));
                }
            } else if (answer->nlmsg_type == NLMSG_OVERRUN) {
                throw VasumException("Receive failed: data lost");
            }
        }
        if (lastOk == NULL) {
            LOGE("Something went terribly wrong. Check vsm_recvmsg function");
            throw VasumException("Can't receive data from system");
        }
        offset +=  NLMSG_ALIGN(ret);
    } while (lastOk->nlmsg_type != NLMSG_DONE && lastOk->nlmsg_flags & NLM_F_MULTI);

    buf->resize(offset);
    return buf;
}

} //namespace netlink
} //namespace vasum
