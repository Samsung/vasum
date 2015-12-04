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
 * @defgroup libcargo-ipc libcargo-ipc
 * @brief   Handling client connections
 */

#ifndef CARGO_IPC_CLIENT_HPP
#define CARGO_IPC_CLIENT_HPP

#include "cargo-ipc/internals/processor.hpp"
#include "cargo-ipc/types.hpp"
#include "cargo-ipc/result.hpp"
#include "epoll/event-poll.hpp"
#include "logger/logger.hpp"

#include <string>

namespace cargo {
namespace ipc {

/**
 * @brief This class wraps communication via UX sockets for client applications.
 * It uses serialization mechanism from Cargo.
 *
 * @code
 * // eventPoll - epoll wrapper class
 * // address - server socket address
 * cargo::ipc::epoll::EventPoll examplePoll;
 * cargo::ipc::Client myClient(examplePoll, address);
 * myClient.start(); // connect to the service
 * // call method synchronously
 * const auto result = mClient.callSync<api::Void, api::ZoneIds>(
 *                            api::cargo::ipc::METHOD_GET_ZONE_ID_LIST,
 *                            std::make_shared<api::Void>());
 * // call method asynchronously
 * // first: declare lambda function to call on completion
 * auto asyncResult = [result](cargo::ipc::Result<api::Void>&& out) {
 *      if (out.isValid()) {
 *          // got successful response!
 *      } else {
 *          // throw expection - one that came back or default
 *          out.rethrow()
 *      }
 * };
 * std::string id = "example_zone_id";
 * mClient.callAsync<api::ZoneId, api::Void>(api::cargo::ipc::METHOD_DESTROY_ZONE,
 *                            std::make_shared<api::ZoneId>(api::ZoneId{id}),
 *                            asyncResult);
 * @endcode
 *
 * @see libCargo
 * @see cargo::ipc::Processor
 * @see cargo::ipc::epoll::EventPoll
 *
 * @ingroup libcargo-ipc
 */
class Client {
public:
    /**
     * Constructs the Client, but doesn't start it.
     * Once set-up, call start() to connect client to the server.
     *
     * @param eventPoll     event poll
     * @param serverPath    path to the server's socket
     */
    Client(epoll::EventPoll& eventPoll, const std::string& serverPath);
    ~Client();

    /**
     * Copying Client class is prohibited.
     */
    Client(const Client&) = delete;
    /**
     * Copying Client class is prohibited.
     */
    Client& operator=(const Client&) = delete;

    /**
     * Starts processing
     * @note if the Client is already running, it quits immediately (no exception thrown)
     */
    void start();

    /**
    * Is the communication thread running?
    *
    * @return is the communication thread running
    */
    bool isStarted();

    /**
     * Stops processing
     *
     * @param wait      should the call block while waiting for all internals to stop? By default true - do block.
     */
    void stop(bool wait = true);

    /**
     * Set the callback called for each new connection to a peer
     *
     * @param newPeerCallback    the callback to call on new connection event
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
     * Saves the callback connected to the method id.
     * When a message with the given method id is received
     * the data will be parsed and passed to this callback.
     *
     * @param methodID          API dependent id of the method
     * @param method            method handling implementation
     * @tparam SentDataType     data type to send
     * @tparam ReceivedDataType data type to receive
     */
    template<typename SentDataType, typename ReceivedDataType>
    void setMethodHandler(const MethodID methodID,
                          const typename MethodHandler<SentDataType, ReceivedDataType>::type& method);

    /**
     * Saves the callback connected to the method id.
     * When a message with the given method id is received
     * the data will be parsed and passed to this callback.
     *
     * @param methodID          API dependent id of the method
     * @param signal            data processing callback
     * @tparam ReceivedDataType data type to receive
     */
    template<typename ReceivedDataType>
    void setSignalHandler(const MethodID methodID,
                          const typename SignalHandler<ReceivedDataType>::type& signal);

    /**
     * Removes the callback associated with specific method id.
     *
     * @param methodID          API dependent id of the method
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
     * @param data                  data to send
     * @param timeoutMS             optional, how long to wait for the return value before throw (milliseconds, default: 5000)
     * @tparam SentDataType         data type to send
     * @tparam ReceivedDataType     data type to receive
     * @return pointer to the call result data
     */
    template<typename SentDataType, typename ReceivedDataType>
    std::shared_ptr<ReceivedDataType> callSync(const MethodID methodID,
                                               const std::shared_ptr<SentDataType>& data,
                                               unsigned int timeoutMS = 5000);

    /**
     * Asynchronous method call. The return callback will be called on
     * return data arrival. It will be run in the PROCESSOR thread.
     *
     * @param methodID               API dependent id of the method
     * @param data                   data to send
     * @param resultCallback         callback processing the return data
     * @tparam SentDataType          data type to send
     * @tparam ReceivedDataType      data type to receive
     */
    template<typename SentDataType, typename ReceivedDataType>
    void callAsync(const MethodID methodID,
                   const std::shared_ptr<SentDataType>& data,
                   const typename ResultHandler<ReceivedDataType>::type& resultCallback = nullptr);


    template<typename SentDataType, typename ReceivedDataType>
    void callAsyncFromCallback(const MethodID methodID,
                               const std::shared_ptr<SentDataType>& data,
                               const typename ResultHandler<ReceivedDataType>::type& resultCallback = nullptr);

    /**
    * Send a signal to the peer.
    * There is no return value from the peer
    * Sends any data only if a peer registered this a signal
    *
    * @param methodID           API dependent id of the method
    * @param data               data to send
    * @tparam SentDataType      data type to send
    */
    template<typename SentDataType>
    void signal(const MethodID methodID,
                const std::shared_ptr<SentDataType>& data);

private:
    epoll::EventPoll& mEventPoll;
    PeerID mServiceID;
    internals::Processor mProcessor;
    std::string mSocketPath;

    void handle(const FileDescriptor fd, const epoll::Events pollEvents);

};

template<typename SentDataType, typename ReceivedDataType>
void Client::setMethodHandler(const MethodID methodID,
                              const typename MethodHandler<SentDataType, ReceivedDataType>::type& method)
{
    LOGS("Client setMethodHandler, methodID: " << methodID);
    mProcessor.setMethodHandler<SentDataType, ReceivedDataType>(methodID, method);
}

template<typename ReceivedDataType>
void Client::setSignalHandler(const MethodID methodID,
                              const typename SignalHandler<ReceivedDataType>::type& handler)
{
    LOGS("Client setSignalHandler, methodID: " << methodID);
    mProcessor.setSignalHandler<ReceivedDataType>(methodID, handler);
}

template<typename SentDataType, typename ReceivedDataType>
std::shared_ptr<ReceivedDataType> Client::callSync(const MethodID methodID,
                                                   const std::shared_ptr<SentDataType>& data,
                                                   unsigned int timeoutMS)
{
    LOGS("Client callSync, methodID: " << methodID << ", timeoutMS: " << timeoutMS);
    return mProcessor.callSync<SentDataType, ReceivedDataType>(methodID, mServiceID, data, timeoutMS);
}

template<typename SentDataType, typename ReceivedDataType>
void Client::callAsync(const MethodID methodID,
                       const std::shared_ptr<SentDataType>& data,
                       const typename ResultHandler<ReceivedDataType>::type& resultCallback)
{
    LOGS("Client callAsync, methodID: " << methodID);
    mProcessor.callAsync<SentDataType,
                         ReceivedDataType>(methodID,
                                           mServiceID,
                                           data,
                                           resultCallback);
}

template<typename SentDataType, typename ReceivedDataType>
void Client::callAsyncFromCallback(const MethodID methodID,
                                   const std::shared_ptr<SentDataType>& data,
                                   const typename ResultHandler<ReceivedDataType>::type& resultCallback)
{
    LOGS("Client callAsyncFromCallback, methodID: " << methodID);
    mProcessor.callAsyncNonBlock<SentDataType,
                                 ReceivedDataType>(methodID,
                                                   mServiceID,
                                                   data,
                                                   resultCallback);
}

template<typename SentDataType>
void Client::signal(const MethodID methodID,
                    const std::shared_ptr<SentDataType>& data)
{
    LOGS("Client signal, methodID: " << methodID);
    mProcessor.signal<SentDataType>(methodID, data);
}

} // namespace ipc
} // namespace cargo

#endif // CARGO_IPC_CLIENT_HPP
