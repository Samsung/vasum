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

#include "ipc/exception.hpp"
#include "ipc/internals/socket.hpp"
#include "ipc/internals/utils.hpp"
#include "logger/logger.hpp"

#include <systemd/sd-daemon.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>


namespace security_containers {
namespace ipc {

namespace {
const int MAX_QUEUE_LENGTH = 1000;
}

Socket::Socket(int socketFD)
    : mFD(socketFD)
{
}

Socket::Socket(Socket&& socket)
    : mFD(socket.mFD)
{
    socket.mFD = -1;
}

Socket::~Socket()
{
    try {
        ipc::close(mFD);
    } catch (IPCException& e) {
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
        LOGE("Error in accept: " << std::string(strerror(errno)));
        IPCException("Error in accept: " + std::string(strerror(errno)));
    }
    return std::make_shared<Socket>(sockfd);
}

void Socket::write(const void* bufferPtr, const size_t size) const
{
    Guard guard(mCommunicationMutex);
    ipc::write(mFD, bufferPtr, size);
}

void Socket::read(void* bufferPtr, const size_t size) const
{
    Guard guard(mCommunicationMutex);
    ipc::read(mFD, bufferPtr, size);
}

int Socket::getSystemdSocket(const std::string& path)
{
    int n = ::sd_listen_fds(-1 /*Block further calls to sd_listen_fds*/);
    if (n < 0) {
        LOGE("sd_listen_fds fails with errno: " + n);
        throw IPCException("sd_listen_fds fails with errno: " + n);
    }

    for (int fd = SD_LISTEN_FDS_START;
            fd < SD_LISTEN_FDS_START + n;
            ++fd) {
        if (0 < ::sd_is_socket_unix(SD_LISTEN_FDS_START, SOCK_STREAM, 1, path.c_str(), 0)) {
            return fd;
        }
    }
    LOGW("No usable sockets were passed by systemd.");
    return -1;
}

int Socket::createDomainSocket(const std::string& path)
{
    // Isn't the path too long?
    if (path.size() >= sizeof(sockaddr_un::sun_path)) {
        LOGE("Socket's path too long");
        throw IPCException("Socket's path too long");
    }

    int sockfd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd == -1) {
        LOGE("Error in socket: " + std::string(strerror(errno)));
        throw IPCException("Error in socket: " + std::string(strerror(errno)));
    }

    ::sockaddr_un serverAddress;
    serverAddress.sun_family = AF_UNIX;
    ::strncpy(serverAddress.sun_path, path.c_str(), sizeof(sockaddr_un::sun_path));
    unlink(serverAddress.sun_path);

    // Everybody can access the socket
    // TODO: Use SMACK to guard the socket
    if (-1 == ::bind(sockfd,
                     reinterpret_cast<struct sockaddr*>(&serverAddress),
                     sizeof(struct sockaddr_un))) {
        std::string message = strerror(errno);
        ::close(sockfd);
        LOGE("Error in bind: " << message);
        IPCException("Error in bind: " + message);
    }

    if (-1 == ::listen(sockfd,
                       MAX_QUEUE_LENGTH)) {
        std::string message = strerror(errno);
        ::close(sockfd);
        LOGE("Error in listen: " << message);
        IPCException("Error in listen: " + message);
    }

    return sockfd;
}

Socket Socket::createSocket(const std::string& path)
{
    // Initialize a socket
    int fd = getSystemdSocket(path);
    fd = fd != -1 ? fd : createDomainSocket(path);

    return Socket(fd);
}

Socket Socket::connectSocket(const std::string& path)
{
    // Isn't the path too long?
    if (path.size() >= sizeof(sockaddr_un::sun_path)) {
        LOGE("Socket's path too long");
        throw IPCException("Socket's path too long");
    }

    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd == -1) {
        LOGE("Error in socket: " + std::string(strerror(errno)));
        throw IPCException("Error in socket: " + std::string(strerror(errno)));
    }

    sockaddr_un serverAddress;
    serverAddress.sun_family = AF_UNIX;
    strncpy(serverAddress.sun_path, path.c_str(), sizeof(sockaddr_un::sun_path));

    if (-1 == connect(fd,
                      reinterpret_cast<struct sockaddr*>(&serverAddress),
                      sizeof(struct sockaddr_un))) {
        ::close(fd);
        LOGE("Error in connect: " + std::string(strerror(errno)));
        throw IPCException("Error in connect: " + std::string(strerror(errno)));
    }

    return Socket(fd);
}

} // namespace ipc
} // namespace security_containers
