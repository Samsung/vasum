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
 * @brief   Declaration of the cargo IPC handling class
 */

#ifndef CARGO_IPC_SERVICE_HPP
#define CARGO_IPC_SERVICE_HPP

#include "cargo-ipc/internals/processor.hpp"
#include "cargo-ipc/internals/acceptor.hpp"
#include "cargo-ipc/types.hpp"
#include "cargo-ipc/result.hpp"
#include "epoll/event-poll.hpp"
#include "logger/logger.hpp"

#include <string>

namespace cargo {
namespace ipc {


/**
 * @brief This class wraps communication via UX sockets.
 * It uses serialization mechanism from Config.
 *
 * @code
 * // eventPoll - epoll wrapper class
 * // address - server socket address
 * cargo::ipc::epoll::EventPoll examplePoll;
 * // create callbacks for connected / disconnected events
 * cargo::ipc::PeerCallback connectedCallback = [this](const cargo::ipc::PeerID peerID,
 *                                              const cargo::ipc::FileDescriptor) {
 *     // new connection came!
 * };
 * cargo::ipc::PeerCallback disconnectedCallback = [this](const cargo::ipc::PeerID peerID,
 *                                                 const cargo::ipc::FileDescriptor) {
 *     // connection disconnected!
 * };
 * // create the service
 * cargo::ipc::Service myService(eventPoll, "/tmp/example_service.socket",
 *                        connectedCallback, disconnectedCallback));
 * // add example method handler
 * auto exampleMethodHandler = [&](const PeerID, std::shared_ptr<RecvData>& data, MethodResult::Pointer methodResult) {
 *     // got example method call! Incoming data in "data" argument
 *     // respond with some data
 *     auto returnData = std::make_shared<SendData>(data->intVal);
 *     methodResult->set(returnData);
 * };
 * const MethodID exampleMethodID = 1234;
 * myService.setMethodHandler<SendData, RecvData>(exampleMethodID, exampleMethodHandler);
 * myService.start(); // start the service, clients may connect via /tmp/example_service.socket
 * @endcode
 *
 * @see libCargo
 * @see cargo::ipc::Processor
 *
 * @ingroup libcargo-ipc
 */
class Service {
public:
    /**
     * Constructs the Service, but doesn't start it.
     * The object is ready to add methods.
     * Once set-up, call start() to start the service.
     *
     * @param eventPoll             event poll
     * @param path                  path to the socket
     * @param addPeerCallback       optional on new peer connection callback
     * @param removePeerCallback    optional on peer removal callback
     */
    Service(epoll::EventPoll& eventPoll,
            const std::string& path,
            const PeerCallback& addPeerCallback = nullptr,
            const PeerCallback& removePeerCallback = nullptr);
    virtual ~Service();

    /**
     * Copying Service class is prohibited.
     */
    Service(const Service&) = delete;
    /**
     * Copying Service class is prohibited.
     */
    Service& operator=(const Service&) = delete;

    /**
     * Starts processing
     * @note if the service is already running, it quits immediately (no exception thrown)
     */
    void start();

    /**
    * @return is the communication thread running
    */
    bool isStarted();

    /**
     * Stops all working threads
     *
     * @param wait      should the call block while waiting for all internals to stop? By default true - do block.
     */
    void stop(bool wait = true);

    /**
    * Set the callback called for each new connection to a peer
    *
    * @param newPeerCallback        the callback to call on new connection event
    * @note if callback is already set, it will be overridden
    */
    void setNewPeerCallback(const PeerCallback& newPeerCallback);

    /**
     * Set the callback called when connection to a peer is lost
     *
     * @param removedPeerCallback   the callback to call on peer disconnected event
     * @note if callback is already set, it will be overridden
     */
    void setRemovedPeerCallback(const PeerCallback& removedPeerCallback);

    /**
     * Saves the callbacks connected to the method id.
     * When a message with the given method id is received,
     * the data will be passed to the serialization callback through file descriptor.
     *
     * Then the process callback will be called with the parsed data.
     *
     * @param methodID              API dependent id of the method
     * @param method                data processing callback
     * @tparam SentDataType         data type to send
     * @tparam ReceivedDataType     data type to receive
     */
    template<typename SentDataType, typename ReceivedDataType>
    void setMethodHandler(const MethodID methodID,
                          const typename MethodHandler<SentDataType, ReceivedDataType>::type& method);

    /**
     * Saves the callbacks connected to the method id.
     * When a message with the given method id is received,
     * the data will be passed to the serialization callback through file descriptor.
     *
     * Then the process callback will be called with the parsed data.
     * There is no return data to send back.
     *
     * Adding signal sends a registering message to all peers
     *
     * @param methodID              API dependent id of the method
     * @param handler               data processing callback
     * @tparam ReceivedDataType     data type to receive
     */
    template<typename ReceivedDataType>
    void setSignalHandler(const MethodID methodID,
                          const typename SignalHandler<ReceivedDataType>::type& handler);

    /**
     * Removes the callback associated with specific method id.
     *
     * @param methodID              API dependent id of the method
     * @see setMethodHandler()
     * @see setSignalHandler()
     */
    void removeMethod(const MethodID methodID);

    /**
     * @param methodID MethodID defined in the user's API
     * @return is methodID handled by a signal or method
     */
    bool isHandled(const MethodID methodID);

    /**
     * Synchronous method call.
     *
     * @param methodID              API dependent id of the method
     * @param peerID                id of the peer
     * @param data                  data to send
     * @param timeoutMS             optional, how long to wait for the return value before throw (milliseconds, default: 5000)
     * @tparam SentDataType         data type to send
     * @tparam ReceivedDataType     data type to receive
     * @return pointer to the call result data
     */
    template<typename SentDataType, typename ReceivedDataType>
    std::shared_ptr<ReceivedDataType> callSync(const MethodID methodID,
                                               const PeerID& peerID,
                                               const std::shared_ptr<SentDataType>& data,
                                               unsigned int timeoutMS = 5000);

    /**
     * Asynchronous method call. The return callback will be called on
     * return data arrival. It will be run in the PROCESSOR thread.
     *
     * @param methodID              API dependent id of the method
     * @param peerID                id of the peer
     * @param data                  data to send
     * @param resultCallback        callback processing the return data
     * @tparam SentDataType         data type to send
     * @tparam ReceivedDataType     data type to receive
     */
    template<typename SentDataType, typename ReceivedDataType>
    void callAsync(const MethodID methodID,
                   const PeerID& peerID,
                   const std::shared_ptr<SentDataType>& data,
                   const typename ResultHandler<ReceivedDataType>::type& resultCallback =  nullptr);

    template<typename SentDataType, typename ReceivedDataType>
    void callAsyncFromCallback(const MethodID methodID,
                               const PeerID& peerID,
                               const std::shared_ptr<SentDataType>& data,
                               const typename ResultHandler<ReceivedDataType>::type& resultCallback  =  nullptr);

    /**
    * Send a signal to the peer.
    * There is no return value from the peer
    * Sends any data only if a peer registered this a signal
    *
    * @param methodID               API dependent id of the method
    * @param data                   data to send
    * @tparam SentDataType          data type to send
    */
    template<typename SentDataType>
    void signal(const MethodID methodID,
                const std::shared_ptr<SentDataType>& data);
private:
    epoll::EventPoll& mEventPoll;
    internals::Processor mProcessor;
    internals::Acceptor mAcceptor;

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
                                                    const PeerID& peerID,
                                                    const std::shared_ptr<SentDataType>& data,
                                                    unsigned int timeoutMS)
{
    LOGS("Service callSync, methodID: " << methodID
         << ", peerID: " << shortenPeerID(peerID)
         << ", timeoutMS: " << timeoutMS);
    return mProcessor.callSync<SentDataType, ReceivedDataType>(methodID, peerID, data, timeoutMS);
}

template<typename SentDataType, typename ReceivedDataType>
void Service::callAsync(const MethodID methodID,
                        const PeerID& peerID,
                        const std::shared_ptr<SentDataType>& data,
                        const typename ResultHandler<ReceivedDataType>::type& resultCallback)
{
    LOGS("Service callAsync, methodID: " << methodID << ", peerID: " << shortenPeerID(peerID));
    mProcessor.callAsync<SentDataType,
                         ReceivedDataType>(methodID,
                                           peerID,
                                           data,
                                           resultCallback);
}

template<typename SentDataType, typename ReceivedDataType>
void Service::callAsyncFromCallback(const MethodID methodID,
                                    const PeerID& peerID,
                                    const std::shared_ptr<SentDataType>& data,
                                    const typename ResultHandler<ReceivedDataType>::type& resultCallback)
{
    LOGS("Service callAsyncFromCallback, methodID: " << methodID
                                     << ", peerID: " << shortenPeerID(peerID));
    mProcessor.callAsyncNonBlock<SentDataType,
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
} // namespace cargo

#endif // CARGO_IPC_SERVICE_HPP
