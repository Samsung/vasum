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
const int UNIX_SOCKET_PROTOCOL = 0;

void setFdOptions(const int fd)
{
    // Prevent from inheriting fd by zones
    if (-1 == ::fcntl(fd, F_SETFD, FD_CLOEXEC)) {
        const std::string msg = "Error in fcntl: " + getSystemErrorMessage();
        LOGE(msg);
        throw IPCException(msg);
    }
}

std::unique_ptr<::addrinfo, void(*)(::addrinfo*)> getAddressInfo(const std::string& host,
                                                                 const std::string& port)
{
    ::addrinfo* addressInfo;

    const char* chost = host.empty() ? nullptr : host.c_str();
    const char* cport = port.empty() ? nullptr : port.c_str();

    int ret = ::getaddrinfo(chost, cport, nullptr, &addressInfo);
    if (ret != 0) {
        const std::string msg = "Failed to get address info: " + std::string(::gai_strerror(ret));
        LOGE(msg);
        throw IPCException(msg);
    }

    return std::unique_ptr<::addrinfo, void(*)(::addrinfo*)>(addressInfo, ::freeaddrinfo);
}

void connect(const int socket,
             const ::sockaddr* address,
             const ::socklen_t addressLength,
             const unsigned int timeoutMS)
{
    auto deadline = std::chrono::steady_clock::now() +
                    std::chrono::milliseconds(timeoutMS);

    // There's a race between connect() in one peer and listen() in the other.
    // We'll retry connect if no one is listening.
    do {
        if (-1 != ::connect(socket,
                            address,
                            addressLength)) {
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

int getSocketFd(const int family, const int type, const int protocol)
{
    int fd = ::socket(family, type, protocol);
    if (fd == -1) {
        const std::string msg = "Error in socket: " + getSystemErrorMessage();
        LOGE(msg);
        throw IPCSocketException(errno, msg);
    }
    setFdOptions(fd);

    return fd;
}

int getConnectedFd(const int family,
                   const int type,
                   const int protocol,
                   const ::sockaddr* address,
                   const ::socklen_t addressLength,
                   const int timeoutMs)
{
    int fd = getSocketFd(family, type, protocol);

    connect(fd, address, addressLength, timeoutMs);

    // Nonblocking socket
    int flags = ::fcntl(fd, F_GETFL, 0);
    if (-1 == ::fcntl(fd, F_SETFL, flags | O_NONBLOCK)) {
        utils::close(fd);
        const std::string msg = "Error in fcntl: " + getSystemErrorMessage();
        LOGE(msg);
        throw IPCException(msg);
    }

    return fd;
}

int getBoundFd(const int family,
               const int type,
               const int protocol,
               const ::sockaddr* address,
               const ::socklen_t addressLength)
{
    int fd = getSocketFd(family, type, protocol);

    // Ensure address doesn't exist before bind() to avoid errors
    ::unlink(reinterpret_cast<const ::sockaddr_un*>(address)->sun_path);

    if (-1 == ::bind(fd, address, addressLength)) {
        utils::close(fd);
        const std::string msg = "Error in bind: " + getSystemErrorMessage();
        LOGE(msg);
        throw IPCException(msg);
    }

    if (-1 == ::listen(fd,
                       MAX_QUEUE_LENGTH)) {
        utils::close(fd);
        const std::string msg = "Error in listen: " + getSystemErrorMessage();
        LOGE(msg);
        throw IPCException(msg);
    }

    return fd;
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

Socket::Type Socket::getType() const
{
    int family;
    socklen_t length = sizeof(family);

    if (::getsockopt(mFD, SOL_SOCKET, SO_DOMAIN, &family, &length)) {
        if (errno == EBADF) {
            return Type::INVALID;
        } else {
            const std::string msg = "Error getting socket type: " + getSystemErrorMessage();
            LOGE(msg);
            throw IPCException(msg);
        }
    }


    if (family == AF_UNIX || family == AF_LOCAL) {
        return Type::UNIX;
    }

    if (family == AF_INET || family == AF_INET6) {
        return Type::INET;
    }

    return Type::INVALID;
}

unsigned short Socket::getPort() const
{
    ::sockaddr_storage address = {0, 0, {0}};
    ::socklen_t length = sizeof(address);
    if (::getsockname(mFD, reinterpret_cast<sockaddr*>(&address), &length) != 0) {
        const std::string msg = "Failed to get socked address: " + getSystemErrorMessage();
        LOGE(msg);
        throw IPCException(msg);
    }

    if (length == sizeof(sockaddr_in)) {
        return ntohs(reinterpret_cast<const sockaddr_in*>(&address)->sin_port);
    } else {
        return ntohs(reinterpret_cast<const sockaddr_in6*>(&address)->sin6_port);
    }
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

    ::sockaddr_un serverAddress;
    serverAddress.sun_family = AF_UNIX;
    ::strncpy(serverAddress.sun_path, path.c_str(), path.length() + 1);

    return getBoundFd(AF_UNIX,
                      SOCK_STREAM,
                      UNIX_SOCKET_PROTOCOL,
                      reinterpret_cast<struct sockaddr*>(&serverAddress),
                      sizeof(struct sockaddr_un));
}

Socket Socket::createUNIX(const std::string& path)
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

Socket Socket::createINET(const std::string& host, const std::string& service)
{
    auto address = getAddressInfo(host, service);

    int fd = getBoundFd(address->ai_family,
                        address->ai_socktype,
                        address->ai_protocol,
                        address->ai_addr,
                        address->ai_addrlen);

    return Socket(fd);
}

Socket Socket::connectUNIX(const std::string& path, const int timeoutMs)
{
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

    int fd = getConnectedFd(AF_UNIX,
                            SOCK_STREAM,
                            UNIX_SOCKET_PROTOCOL,
                            reinterpret_cast<struct sockaddr*>(&serverAddress),
                            sizeof(struct ::sockaddr_un),
                            timeoutMs);

    return Socket(fd);
}

Socket Socket::connectINET(const std::string& host, const std::string& service, const int timeoutMs)
{
    auto addressInfo = getAddressInfo(host, service);

    int fd = getConnectedFd(addressInfo->ai_family,
                            addressInfo->ai_socktype,
                            addressInfo->ai_protocol,
                            reinterpret_cast<::sockaddr*>(addressInfo->ai_addr),
                            addressInfo->ai_addrlen,
                            timeoutMs);

    return Socket(fd);
}

} // namespace internals
} // namespace ipc
} // namespace cargo
