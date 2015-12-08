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

#ifndef CARGO_IPC_INTERNALS_SOCKET_HPP
#define CARGO_IPC_INTERNALS_SOCKET_HPP

#include <string>
#include <mutex>
#include <memory>
#include <netdb.h>

namespace cargo {
namespace ipc {
namespace internals {

/**
 * This class wraps all operations possible to do with a socket.
 *
 * It has operations both for client and server application.
 */
class Socket {
public:
    enum class Type : int8_t {
        INVALID,
        UNIX,
        INET
    };

    typedef std::unique_lock<std::recursive_mutex> Guard;

    /**
     * Default constructor.
     * If socketFD is passed then it's passed by the Socket
     *
     * @param socketFD socket obtained outside the class.
     */
    explicit Socket(int socketFD = -1);
    Socket(Socket&& socket) noexcept;
    ~Socket() noexcept;

    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;
    Socket& operator=(Socket&&) = delete;

    /**
     * @return reference to the socket's file descriptor
     */
    int getFD() const;

    /**
     * Write data using the file descriptor
     *
     * @param bufferPtr buffer with the data
     * @param size size of the buffer
     */
    void write(const void* bufferPtr, const size_t size) const;

    /**
     * Reads a value of the given type.
     *
     * @param bufferPtr buffer with the data
     * @param size size of the buffer
     */
    void read(void* bufferPtr, const size_t size) const;

    /**
     * Accepts connection. Used by a server application.
     * Blocking, called by a server.
     */
    std::shared_ptr<Socket> accept();

    /**
     * Returns the socket type based on it's domain.
     */
    Type getType() const;

    /**
     * Returns a port associated with the socket.
     */
    unsigned short getPort() const;

    /**
     * Prepares UNIX socket for accepting connections.
     * Called by a server.
     *
     * @param path path to the socket
     * @return created socket
     */
    static Socket createUNIX(const std::string& path);

    /**
     * Prepares INET socket for accepting connections.
     * Called by a server.
     *
     * @param host hostname or ip address
     * @param service port number or service name
     * @return created socket
     */
    static Socket createINET(const std::string& host, const std::string& service);

    /**
     * Connects to an UNIX socket. Called as a client.
     *
     * @param path path to the socket
     * @return connected socket
     */
    static Socket connectUNIX(const std::string& path, const int timeoutMs = 1000);

    /**
     * Connects to an INET socket. Called as a client.
     *
     * @param host hostname or ip address
     * @param service port number or service name
     * @return connected socket
     */
    static Socket connectINET(const std::string& host,
                              const std::string& service,
                              const int timeoutMs = 1000);

private:
    int mFD;
    mutable std::recursive_mutex mCommunicationMutex;

    static int createSocketInternal(const std::string& path);
    static int getSystemdSocketInternal(const std::string& path);
};

} // namespace internals
} // namespace ipc
} // namespace cargo

#endif // CARGO_IPC_INTERNALS_SOCKET_HPP
