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
 * @brief   Declaration of the IPC handling class
 */

#ifndef COMMON_IPC_SERVICE_HPP
#define COMMON_IPC_SERVICE_HPP

#include "ipc/internals/processor.hpp"
#include "ipc/internals/acceptor.hpp"
#include "ipc/types.hpp"
#include "logger/logger.hpp"

#include <string>

namespace security_containers {
namespace ipc {


/**
 * This class wraps communication via UX sockets.
 * It uses serialization mechanism from libConfig.
 *
 * There are two working threads:
 * - ACCEPTOR accepts incoming connections and passes them to PROCESSOR
 * - PROCESSOR is responsible for the communication and calling the callbacks
 *
 * For message format @see ipc::Processor
 */
class Service {
public:
    typedef Processor::PeerCallback PeerCallback;
    typedef Processor::PeerID PeerID;
    typedef Processor::MethodID MethodID;

    /**
     * @param path path to the socket
     */
    Service(const std::string& path,
            const PeerCallback& addPeerCallback = nullptr,
            const PeerCallback& removePeerCallback = nullptr);
    ~Service();

    Service(const Service&) = delete;
    Service& operator=(const Service&) = delete;

    /**
     * Starts the worker and acceptor threads
     */
    void start();

    /**
     * Stops all working threads
     */
    void stop();

    /**
     * Saves the callback connected to the method id.
     * When a message with the given method id is received
     * the data will be parsed and passed to this callback.
     *
     * @param methodID API dependent id of the method
     * @param methodCallback method handling implementation
     */
    template<typename SentDataType, typename ReceivedDataType>
    void addMethodHandler(const MethodID methodID,
                          const typename MethodHandler<SentDataType, ReceivedDataType>::type& method);

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
     * @param sendCallback callback for data serialization
     * @param resultCallback callback for result serialization and handling
     */
    template<typename SentDataType, typename ReceivedDataType>
    void callAsync(const MethodID methodID,
                   const PeerID peerID,
                   const std::shared_ptr<SentDataType>& data,
                   const typename ResultHandler<ReceivedDataType>::type& resultCallback);

private:
    typedef std::lock_guard<std::mutex> Lock;
    Processor mProcessor;
    Acceptor mAcceptor;
};


template<typename SentDataType, typename ReceivedDataType>
void Service::addMethodHandler(const MethodID methodID,
                               const typename MethodHandler<SentDataType, ReceivedDataType>::type& method)
{
    LOGD("Adding method with id " << methodID);
    mProcessor.addMethodHandler<SentDataType, ReceivedDataType>(methodID, method);
    LOGD("Added method with id " << methodID);
}

template<typename SentDataType, typename ReceivedDataType>
std::shared_ptr<ReceivedDataType> Service::callSync(const MethodID methodID,
                                                    const PeerID peerID,
                                                    const std::shared_ptr<SentDataType>& data,
                                                    unsigned int timeoutMS)
{
    LOGD("Sync calling method: " << methodID << " for user: " << peerID);
    return mProcessor.callSync<SentDataType, ReceivedDataType>(methodID, peerID, data, timeoutMS);
}

template<typename SentDataType, typename ReceivedDataType>
void Service::callAsync(const MethodID methodID,
                        const PeerID peerID,
                        const std::shared_ptr<SentDataType>& data,
                        const typename ResultHandler<ReceivedDataType>::type& resultCallback)
{
    LOGD("Async calling method: " << methodID << " for user: " << peerID);
    mProcessor.callAsync<SentDataType,
                         ReceivedDataType>(methodID,
                                           peerID,
                                           data,
                                           resultCallback);
    LOGD("Async called method: " << methodID << "for user: " << peerID);
}


} // namespace ipc
} // namespace security_containers

#endif // COMMON_IPC_SERVICE_HPP
