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
 * @author  Jan Olszak (j.olszak@samsung.com)
 * @brief   Linux socket wrapper
 */

#include "config.hpp"

#include "cargo-ipc/exception.hpp"
#include "cargo-ipc/internals/socket.hpp"
#include "utils/fd-utils.hpp"
#include "utils/exception.hpp"
#include "logger/logger.hpp"

#ifdef HAVE_SYSTEMD
#include <systemd/sd-daemon.h>
#endif // HAVE_SYSTEMD
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <thread>

using namespace utils;

namespace cargo {
namespace ipc {
namespace internals {

namespace {
const int MAX_QUEUE_LENGTH = 1000;
const int RETRY_CONNECT_STEP_MS = 10;

void setFdOptions(int fd)
{
    // Prevent from inheriting fd by zones
    if (-1 == ::fcntl(fd, F_SETFD, FD_CLOEXEC)) {
        const std::string msg = "Error in fcntl: " + getSystemErrorMessage();
        LOGE(msg);
        throw IPCException(msg);
    }
}

void connect(int socket, const std::string& path, const unsigned int timeoutMS)
{
    auto deadline = std::chrono::steady_clock::now() +
                    std::chrono::milliseconds(timeoutMS);

    // Isn't the path too long?
    if (path.size() >= sizeof(::sockaddr_un::sun_path)) {
        const std::string msg = "Socket's path too long";
        LOGE(msg);
        throw IPCException(msg);
    }

    // Fill address
    struct ::sockaddr_un serverAddress;
    serverAddress.sun_family = AF_UNIX;
    ::strncpy(serverAddress.sun_path, path.c_str(), sizeof(::sockaddr_un::sun_path));

    // There's a race between connect() in one peer and listen() in the other.
    // We'll retry connect if no one is listening.
    do {
        if (-1 != ::connect(socket,
                            reinterpret_cast<struct sockaddr*>(&serverAddress),
                            sizeof(struct ::sockaddr_un))) {
            return;
        }

        if (errno == ECONNREFUSED || errno == EAGAIN || errno == EINTR) {
            // No one is listening, so sleep and retry
            LOGW("No one listening on the socket, retrying");
            std::this_thread::sleep_for(std::chrono::milliseconds(RETRY_CONNECT_STEP_MS));
            continue;
        }

        // Error
        utils::close(socket);
        const std::string msg = "Error in connect: " + getSystemErrorMessage();
        LOGE(msg);
        throw IPCException(msg);

    } while (std::chrono::steady_clock::now() < deadline);

    const std::string msg = "Timeout in connect";
    LOGE(msg);
    throw IPCException(msg);
}

} // namespace

Socket::Socket(int socketFD)
    : mFD(socketFD)
{
}

Socket::Socket(Socket&& socket) noexcept
    : mFD(socket.mFD)
{
    socket.mFD = -1;
}

Socket::~Socket() noexcept
{
    try {
        utils::close(mFD);
    } catch (std::exception& e) {
        LOGE("Error in Socket's destructor: " << e.what());
    }
}

Socket::Guard Socket::getGuard() const
{
    return Guard(mCommunicationMutex);
}

int Socket::getFD() const
{
    return mFD;
}

std::shared_ptr<Socket> Socket::accept()
{
    int sockfd = ::accept(mFD, nullptr, nullptr);
    if (sockfd == -1) {
        const std::string msg = "Error in accept: " + getSystemErrorMessage();
        LOGE(msg);
        throw IPCException(msg);
    }
    setFdOptions(sockfd);
    return std::make_shared<Socket>(sockfd);
}

void Socket::write(const void* bufferPtr, const size_t size) const
{
    Guard guard(mCommunicationMutex);
    utils::write(mFD, bufferPtr, size);
}

void Socket::read(void* bufferPtr, const size_t size) const
{
    Guard guard(mCommunicationMutex);
    utils::read(mFD, bufferPtr, size);
}

#ifdef HAVE_SYSTEMD
int Socket::getSystemdSocketInternal(const std::string& path)
{
    int n = ::sd_listen_fds(-1 /*Block further calls to sd_listen_fds*/);
    if (n < 0) {
        const std::string msg = "sd_listen_fds failed: " + getSystemErrorMessage(-n);
        LOGE(msg);
        throw IPCException(msg);
    }

    for (int fd = SD_LISTEN_FDS_START;
            fd < SD_LISTEN_FDS_START + n;
            ++fd) {
        if (0 < ::sd_is_socket_unix(fd, SOCK_STREAM, 1, path.c_str(), 0)) {
            setFdOptions(fd);
            return fd;
        }
    }
    LOGW("No usable sockets were passed by systemd.");
    return -1;
}
#endif // HAVE_SYSTEMD

int Socket::createSocketInternal(const std::string& path)
{
    // Isn't the path too long?
    if (path.size() >= sizeof(sockaddr_un::sun_path)) {
        const std::string msg = "Socket's path too long";
        LOGE(msg);
        throw IPCException(msg);
    }

    int sockfd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd == -1) {
        const std::string msg = "Error in socket: " + getSystemErrorMessage();
        LOGE(msg);
        throw IPCSocketException(errno, msg);
    }
    setFdOptions(sockfd);

    ::sockaddr_un serverAddress;
    serverAddress.sun_family = AF_UNIX;
    ::strncpy(serverAddress.sun_path, path.c_str(), sizeof(sockaddr_un::sun_path));

    // Ensure address doesn't exist before bind() to avoid errors
    ::unlink(serverAddress.sun_path);

    if (-1 == ::bind(sockfd,
                     reinterpret_cast<struct sockaddr*>(&serverAddress),
                     sizeof(struct sockaddr_un))) {
        utils::close(sockfd);
        const std::string msg = "Error in bind: " + getSystemErrorMessage();
        LOGE(msg);
        throw IPCException(msg);
    }

    if (-1 == ::listen(sockfd,
                       MAX_QUEUE_LENGTH)) {
        utils::close(sockfd);
        const std::string msg = "Error in listen: " + getSystemErrorMessage();
        LOGE(msg);
        throw IPCException(msg);
    }

    return sockfd;
}

Socket Socket::createSocket(const std::string& path)
{
    // Initialize a socket
    int fd;
#ifdef HAVE_SYSTEMD
    fd = getSystemdSocketInternal(path);
    if (fd == -1) {
        fd = createSocketInternal(path);
    }
#else
    fd = createSocketInternal(path);
#endif // HAVE_SYSTEMD

    return Socket(fd);
}

Socket Socket::connectSocket(const std::string& path, const int timeoutMs)
{
    int fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd == -1) {
        const std::string msg = "Error in socket: " + getSystemErrorMessage();
        LOGE(msg);
        throw IPCSocketException(errno, msg);
    }
    setFdOptions(fd);

    connect(fd, path, timeoutMs);

    // Nonblocking socket
    int flags = ::fcntl(fd, F_GETFL, 0);
    if (-1 == ::fcntl(fd, F_SETFL, flags | O_NONBLOCK)) {
        utils::close(fd);
        const std::string msg = "Error in fcntl: " + getSystemErrorMessage();
        LOGE(msg);
        throw IPCException(msg);
    }

    return Socket(fd);
}

} // namespace internals
} // namespace ipc
} // namespace cargo
