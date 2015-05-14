/*
*  Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
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
 * @brief   Declaration of the IPC handling class
 */

#ifndef COMMON_IPC_SERVICE_HPP
#define COMMON_IPC_SERVICE_HPP

#include "ipc/internals/processor.hpp"
#include "ipc/internals/acceptor.hpp"
#include "ipc/types.hpp"
#include "ipc/result.hpp"
#include "epoll/event-poll.hpp"
#include "logger/logger.hpp"

#include <string>

namespace ipc {


/**
 * This class wraps communication via UX sockets.
 * It uses serialization mechanism from Config.
 *
 * For message format @see ipc::Processor
 */
class Service {
public:
    /**
     * @param eventPoll event poll
     * @param path path to the socket
     */
    Service(epoll::EventPoll& eventPoll,
            const std::string& path,
            const PeerCallback& addPeerCallback = nullptr,
            const PeerCallback& removePeerCallback = nullptr);
    ~Service();

    Service(const Service&) = delete;
    Service& operator=(const Service&) = delete;

    /**
     * Starts processing
     */
    void start();

    /**
    * @return is the communication thread running
    */
    bool isStarted();

    /**
     * Stops all working threads
     */
    void stop();

    /**
    * Set the callback called for each new connection to a peer
    *
    * @param newPeerCallback the callback
    */
    void setNewPeerCallback(const PeerCallback& newPeerCallback);

    /**
     * Set the callback called when connection to a peer is lost
     *
     * @param removedPeerCallback the callback
     */
    void setRemovedPeerCallback(const PeerCallback& removedPeerCallback);

    /**
     * Saves the callback connected to the method id.
     * When a message with the given method id is received
     * the data will be parsed and passed to this callback.
     *
     * @param methodID API dependent id of the method
     * @param method method handling implementation
     */
    template<typename SentDataType, typename ReceivedDataType>
    void setMethodHandler(const MethodID methodID,
                          const typename MethodHandler<SentDataType, ReceivedDataType>::type& method);

    /**
     * Saves the callback connected to the method id.
     * When a message with the given method id is received
     * the data will be parsed and passed to this callback.
     *
     * @param methodID API dependent id of the method
     * @param handler handling implementation
     * @tparam ReceivedDataType data type to serialize
     */
    template<typename ReceivedDataType>
    void setSignalHandler(const MethodID methodID,
                          const typename SignalHandler<ReceivedDataType>::type& handler);

    /**
     * Removes the callback
     *
     * @param methodID API dependent id of the method
     */
    void removeMethod(const MethodID methodID);

    /**
     * Synchronous method call.
     *
     * @param methodID API dependent id of the method
     * @param data data to send
     * @return result data
     */
    template<typename SentDataType, typename ReceivedDataType>
    std::shared_ptr<ReceivedDataType> callSync(const MethodID methodID,
                                               const PeerID peerID,
                                               const std::shared_ptr<SentDataType>& data,
                                               unsigned int timeoutMS = 500);

    /**
     * Asynchronous method call. The return callback will be called on
     * return data arrival. It will be run in the PROCESSOR thread.
     *
     *
     * @param methodID API dependent id of the method
     * @param data data to send
     * @param resultCallback callback for result serialization and handling
     */
    template<typename SentDataType, typename ReceivedDataType>
    void callAsync(const MethodID methodID,
                   const PeerID peerID,
                   const std::shared_ptr<SentDataType>& data,
                   const typename ResultHandler<ReceivedDataType>::type& resultCallback);

    /**
    * Send a signal to the peer.
    * There is no return value from the peer
    * Sends any data only if a peer registered this a signal
    *
    * @param methodID API dependent id of the method
    * @param data data to sent
    * @tparam SentDataType data type to send
    */
    template<typename SentDataType>
    void signal(const MethodID methodID,
                const std::shared_ptr<SentDataType>& data);
private:
    epoll::EventPoll& mEventPoll;
    Processor mProcessor;
    Acceptor mAcceptor;

    void handle(const FileDescriptor fd, const epoll::Events pollEvents);
};


template<typename SentDataType, typename ReceivedDataType>
void Service::setMethodHandler(const MethodID methodID,
                               const typename MethodHandler<SentDataType, ReceivedDataType>::type& method)
{
    LOGS("Service setMethodHandler, methodID " << methodID);
    mProcessor.setMethodHandler<SentDataType, ReceivedDataType>(methodID, method);
}

template<typename ReceivedDataType>
void Service::setSignalHandler(const MethodID methodID,
                               const typename SignalHandler<ReceivedDataType>::type& handler)
{
    LOGS("Service setSignalHandler, methodID " << methodID);
    mProcessor.setSignalHandler<ReceivedDataType>(methodID, handler);
}

template<typename SentDataType, typename ReceivedDataType>
std::shared_ptr<ReceivedDataType> Service::callSync(const MethodID methodID,
                                                    const PeerID peerID,
                                                    const std::shared_ptr<SentDataType>& data,
                                                    unsigned int timeoutMS)
{
    LOGS("Service callSync, methodID: " << methodID
         << ", peerID: " << peerID
         << ", timeoutMS: " << timeoutMS);
    return mProcessor.callSync<SentDataType, ReceivedDataType>(methodID, peerID, data, timeoutMS);
}

template<typename SentDataType, typename ReceivedDataType>
void Service::callAsync(const MethodID methodID,
                        const PeerID peerID,
                        const std::shared_ptr<SentDataType>& data,
                        const typename ResultHandler<ReceivedDataType>::type& resultCallback)
{
    LOGS("Service callAsync, methodID: " << methodID << ", peerID: " << peerID);
    mProcessor.callAsync<SentDataType,
                         ReceivedDataType>(methodID,
                                           peerID,
                                           data,
                                           resultCallback);
}

template<typename SentDataType>
void Service::signal(const MethodID methodID,
                     const std::shared_ptr<SentDataType>& data)
{
    LOGS("Service signal, methodID: " << methodID);
    mProcessor.signal<SentDataType>(methodID, data);
}

} // namespace ipc

#endif // COMMON_IPC_SERVICE_HPP
