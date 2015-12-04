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
 * @author Jan Olszak (j.olszak@samsung.com)
 * @brief  Definition of a class for writing and reading data from a file descriptor
 */

#include "config.hpp"

#include "cargo-fd/internals/fdstore.hpp"
#include "cargo/exception.hpp"

#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <chrono>
#include <poll.h>
#include <sys/socket.h>

namespace cargo {

namespace internals {

namespace {

const int ERROR_MESSAGE_BUFFER_CAPACITY = 256;

std::string getSystemErrorMessage()
{
    char buf[ERROR_MESSAGE_BUFFER_CAPACITY];
    return strerror_r(errno, buf, sizeof(buf));
}


void waitForEvent(int fd,
                  short event,
                  const std::chrono::high_resolution_clock::time_point deadline)
{
    // Wait for the rest of the data
    struct pollfd fds[1];
    fds[0].fd = fd;
    fds[0].events = event | POLLHUP;

    for (;;) {
        std::chrono::milliseconds timeoutMS =
            std::chrono::duration_cast<std::chrono::milliseconds>(deadline - std::chrono::high_resolution_clock::now());
        if (timeoutMS.count() < 0) {
            throw CargoException("Timeout");
        }

        int ret = ::poll(fds, 1 /*fds size*/, timeoutMS.count());

        if (ret == -1) {
            if (errno == EINTR) {
                continue;
            }
            throw CargoException("Error in poll: " + getSystemErrorMessage());
        }

        if (ret == 0) {
            throw CargoException("Timeout");
        }

        if (fds[0].revents & POLLHUP) {
            throw CargoException("Peer disconnected");
        }

        // Here Comes the Sun
        break;
    }
}

} // namespace

FDStore::FDStore(int fd)
    : mFD(fd)
{
}

FDStore::FDStore(const FDStore& store)
    : mFD(store.mFD)
{
}

FDStore::~FDStore()
{
}

void FDStore::write(const void* bufferPtr, const size_t size, const unsigned int timeoutMS)
{
    std::chrono::high_resolution_clock::time_point deadline =
        std::chrono::high_resolution_clock::now() +
        std::chrono::milliseconds(timeoutMS);

    size_t nTotal = 0;
    for (;;) {
        ssize_t n  = ::write(mFD,
                             reinterpret_cast<const char*>(bufferPtr) + nTotal,
                             size - nTotal);
        if (n < 0) {
            // Handle errors
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                // Neglected errors
            } else {
                throw CargoException("Error during writing: " + getSystemErrorMessage());
            }
        } else {
            nTotal += n;
            if (nTotal == size) {
                // All data is read, break loop
                break;
            }
        }

        waitForEvent(mFD, POLLOUT, deadline);
    }
}

void FDStore::read(void* bufferPtr, const size_t size, const unsigned int timeoutMS)
{
    std::chrono::high_resolution_clock::time_point deadline =
        std::chrono::high_resolution_clock::now() +
        std::chrono::milliseconds(timeoutMS);

    size_t nTotal = 0;
    for (;;) {
        ssize_t n  = ::read(mFD,
                            reinterpret_cast<char*>(bufferPtr) + nTotal,
                            size - nTotal);
        if (n < 0) {
            // Handle errors
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                // Neglected errors
            } else {
                throw CargoException("Error during reading: " + getSystemErrorMessage());
            }
        } else {
            nTotal += n;
            if (nTotal == size) {
                // All data is read, break loop
                break;
            }
            if (n == 0) {
                throw CargoException("Peer disconnected");
            }
        }

        waitForEvent(mFD, POLLIN, deadline);
    }
}


void FDStore::sendFD(int fd, const unsigned int timeoutMS)
{
    std::chrono::high_resolution_clock::time_point deadline =
        std::chrono::high_resolution_clock::now() +
        std::chrono::milliseconds(timeoutMS);

    // Space for the file descriptor
    union {
        struct cmsghdr cmh;
        char   control[CMSG_SPACE(sizeof(int))];
    } controlUnion;

    // Ensure at least 1 byte is transmited via the socket
    struct iovec iov;
    char buf = '!';
    iov.iov_base = &buf;
    iov.iov_len = sizeof(char);

    // Fill the message to send:
    // The socket has to be connected, so we don't need to specify the name
    struct msghdr msgh;
    ::memset(&msgh, 0, sizeof(msgh));

    // Only iovec to transmit one element
    msgh.msg_iov = &iov;
    msgh.msg_iovlen = 1;

    // Ancillary data buffer
    msgh.msg_control = controlUnion.control;
    msgh.msg_controllen = sizeof(controlUnion.control);

    // Describe the data that we want to send
    struct cmsghdr *cmhp;
    cmhp = CMSG_FIRSTHDR(&msgh);
    cmhp->cmsg_len = CMSG_LEN(sizeof(int));
    cmhp->cmsg_level = SOL_SOCKET;
    cmhp->cmsg_type = SCM_RIGHTS;
    *(reinterpret_cast<int*>(CMSG_DATA(cmhp))) = fd;

    // Send
    for(;;) {
        ssize_t ret = ::sendmsg(mFD, &msgh, MSG_NOSIGNAL);
        if (ret < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                // Neglected errors, retry
            } else {
                throw CargoException("Error during sendmsg: " + getSystemErrorMessage());
            }
        } else if (ret == 0) {
            // Retry the sending
        } else {
            // We send only 1 byte of data. No need to repeat
            break;
        }

        waitForEvent(mFD, POLLOUT, deadline);
    }
}


int FDStore::receiveFD(const unsigned int timeoutMS)
{
    std::chrono::high_resolution_clock::time_point deadline =
        std::chrono::high_resolution_clock::now() +
        std::chrono::milliseconds(timeoutMS);

    // Space for the file descriptor
    union {
        struct cmsghdr cmh;
        char   control[CMSG_SPACE(sizeof(int))];
    } controlUnion;

    // Describe the data that we want to recive
    controlUnion.cmh.cmsg_len = CMSG_LEN(sizeof(int));
    controlUnion.cmh.cmsg_level = SOL_SOCKET;
    controlUnion.cmh.cmsg_type = SCM_RIGHTS;

    // Setup the input buffer
    // Ensure at least 1 byte is transmited via the socket
    char buf;
    struct iovec iov;
    iov.iov_base = &buf;
    iov.iov_len = sizeof(char);

    // Set the ancillary data buffer
    // The socket has to be connected, so we don't need to specify the name
    struct msghdr msgh;
    ::memset(&msgh, 0, sizeof(msgh));

    msgh.msg_iov = &iov;
    msgh.msg_iovlen = 1;

    msgh.msg_control = controlUnion.control;
    msgh.msg_controllen = sizeof(controlUnion.control);

    // Receive
    for(;;) {
        ssize_t ret = ::recvmsg(mFD, &msgh, MSG_WAITALL | MSG_CMSG_CLOEXEC);
        if (ret < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                // Neglected errors, retry
            } else {
                throw CargoException("Error during recvmsg: " + getSystemErrorMessage());
            }
        } else if (ret == 0) {
            throw CargoException("Peer disconnected");
        } else {
            // We receive only 1 byte of data. No need to repeat
            break;
        }

        waitForEvent(mFD, POLLIN, deadline);
    }

    struct cmsghdr *cmhp;
    cmhp = CMSG_FIRSTHDR(&msgh);
    if (cmhp == NULL || cmhp->cmsg_len != CMSG_LEN(sizeof(int))) {
        throw CargoException("Bad cmsg length");
    } else if (cmhp->cmsg_level != SOL_SOCKET) {
        throw CargoException("cmsg_level != SOL_SOCKET");
    } else if (cmhp->cmsg_type != SCM_RIGHTS) {
        throw CargoException("cmsg_type != SCM_RIGHTS");
    }

    return *(reinterpret_cast<int*>(CMSG_DATA(cmhp)));
}

} // namespace internals

} // namespace cargo
