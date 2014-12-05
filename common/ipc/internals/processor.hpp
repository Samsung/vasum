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
 * @brief   Data and event processing thread
 */

#ifndef COMMON_IPC_INTERNALS_PROCESSOR_HPP
#define COMMON_IPC_INTERNALS_PROCESSOR_HPP

#include "ipc/internals/socket.hpp"
#include "ipc/internals/event-queue.hpp"
#include "ipc/internals/call-queue.hpp"
#include "ipc/exception.hpp"
#include "ipc/types.hpp"
#include "config/manager.hpp"
#include "config/fields.hpp"
#include "logger/logger.hpp"

#include <poll.h>

#include <atomic>
#include <condition_variable>
#include <queue>
#include <mutex>
#include <chrono>
#include <vector>
#include <thread>
#include <string>
#include <list>
#include <functional>
#include <unordered_map>

namespace security_containers {
namespace ipc {

const unsigned int DEFAULT_MAX_NUMBER_OF_PEERS = 500;
const unsigned int DEFAULT_METHOD_TIMEOUT = 1000;

/**
* This class wraps communication via UX sockets
*
* It's intended to be used both in Client and Service classes.
* It uses a serialization mechanism from libConfig.
* Library user will only have to pass the types that each call will send and receive
*
* Message format:
* - MethodID  - probably casted enum.
*               MethodID == std::numeric_limits<MethodID>::max() is reserved for return messages
* - MessageID - unique id of a message exchange sent by this object instance. Used to identify reply messages.
* - Rest: The data written in a callback. One type per method.ReturnCallbacks
*
* TODO:
*  - some mutexes may not be needed
*  - synchronous call to many peers
*  - implement HandlerStore class for storing both signals and methods
*  - API for removing signals
*  - implement CallbackStore - thread safe calling/setting callbacks
*  - helper function for removing from unordered map
*  - new way to generate UIDs
*  - callbacks for serialization/parsing
*/
class Processor {
public:
    /**
     * Used to indicate a message with the return value.
     */
    static const MethodID RETURN_METHOD_ID;

    /**
     * Indicates an Processor's internal request/broadcast to register a Signal
     */
    static const MethodID REGISTER_SIGNAL_METHOD_ID;

    /**
     * Constructs the Processor, but doesn't start it.
     * The object is ready to add methods.
     *
     * @param newPeerCallback called when a new peer arrives
     * @param removedPeerCallback called when the Processor stops listening for this peer
     */
    Processor(const PeerCallback& newPeerCallback = nullptr,
              const PeerCallback& removedPeerCallback = nullptr,
              const unsigned int maxNumberOfPeers = DEFAULT_MAX_NUMBER_OF_PEERS);
    ~Processor();

    Processor(const Processor&) = delete;
    Processor(Processor&&) = delete;
    Processor& operator=(const Processor&) = delete;

    /**
     * Start the processing thread.
     * Quits immediately after starting the thread.
     */
    void start();

    /**
     * @return is processor running
     */
    bool isStarted();

    /**
     * Stops the processing thread.
     * No incoming data will be handled after.
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
     * From now on socket is owned by the Processor object.
     * Calls the newPeerCallback.
     *
     * @param socketPtr pointer to the new socket
     * @return peerID of the new socket
     */
    PeerID addPeer(const std::shared_ptr<Socket>& socketPtr);

    /**
     * Request removing peer and wait
     *
     * @param peerID id of the peer
     */
    void removePeer(const PeerID peerID);

    /**
     * Saves the callbacks connected to the method id.
     * When a message with the given method id is received,
     * the data will be passed to the serialization callback through file descriptor.
     *
     * Then the process callback will be called with the parsed data.
     *
     * @param methodID API dependent id of the method
     * @param process data processing callback
     * @tparam SentDataType data type to send
     * @tparam ReceivedDataType data type to receive
     */
    template<typename SentDataType, typename ReceivedDataType>
    void addMethodHandler(const MethodID methodID,
                          const typename MethodHandler<SentDataType, ReceivedDataType>::type& process);

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
     * @param methodID API dependent id of the method
     * @param process data processing callback
     * @tparam ReceivedDataType data type to receive
     */
    template<typename ReceivedDataType>
    void addSignalHandler(const MethodID methodID,
                          const typename SignalHandler<ReceivedDataType>::type& process);

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
     * @param peerID id of the peer
     * @param data data to sent
     * @param timeoutMS how long to wait for the return value before throw
     * @tparam SentDataType data type to send
     * @tparam ReceivedDataType data type to receive
     */
    template<typename SentDataType, typename ReceivedDataType>
    std::shared_ptr<ReceivedDataType> callSync(const MethodID methodID,
                                               const PeerID peerID,
                                               const std::shared_ptr<SentDataType>& data,
                                               unsigned int timeoutMS = 500);

    /**
     * Asynchronous method call
     *
     * @param methodID API dependent id of the method
     * @param peerID id of the peer
     * @param data data to sent
     * @param process callback processing the return data
     * @tparam SentDataType data type to send
     * @tparam ReceivedDataType data type to receive
     */
    template<typename SentDataType, typename ReceivedDataType>
    MessageID callAsync(const MethodID methodID,
                        const PeerID peerID,
                        const std::shared_ptr<SentDataType>& data,
                        const typename ResultHandler<ReceivedDataType>::type& process);


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
    typedef std::function<void(int fd, std::shared_ptr<void>& data)> SerializeCallback;
    typedef std::function<std::shared_ptr<void>(int fd)> ParseCallback;
    typedef std::unique_lock<std::mutex> Lock;

    struct EmptyData {
        CONFIG_REGISTER_EMPTY
    };

    struct RegisterSignalsMessage {
        RegisterSignalsMessage() = default;
        RegisterSignalsMessage(const std::vector<MethodID> ids)
            : ids(ids) {}

        std::vector<MethodID> ids;

        CONFIG_REGISTER
        (
            ids
        )
    };

    struct MethodHandlers {
        MethodHandlers(const MethodHandlers& other) = delete;
        MethodHandlers& operator=(const MethodHandlers&) = delete;
        MethodHandlers() = default;
        MethodHandlers(MethodHandlers&&) = default;
        MethodHandlers& operator=(MethodHandlers &&) = default;

        SerializeCallback serialize;
        ParseCallback parse;
        MethodHandler<void, void>::type method;
    };

    struct SignalHandlers {
        SignalHandlers(const SignalHandlers& other) = delete;
        SignalHandlers& operator=(const SignalHandlers&) = delete;
        SignalHandlers() = default;
        SignalHandlers(SignalHandlers&&) = default;
        SignalHandlers& operator=(SignalHandlers &&) = default;

        ParseCallback parse;
        SignalHandler<void>::type signal;
    };

    struct ReturnCallbacks {
        ReturnCallbacks(const ReturnCallbacks& other) = delete;
        ReturnCallbacks& operator=(const ReturnCallbacks&) = delete;
        ReturnCallbacks() = default;
        ReturnCallbacks(ReturnCallbacks&&) = default;
        ReturnCallbacks& operator=(ReturnCallbacks &&) = default;

        ReturnCallbacks(PeerID peerID, const ParseCallback& parse, const ResultHandler<void>::type& process)
            : peerID(peerID), parse(parse), process(process) {}

        PeerID peerID;
        ParseCallback parse;
        ResultHandler<void>::type process;
    };

    struct SocketInfo {
        SocketInfo(const SocketInfo& other) = delete;
        SocketInfo& operator=(const SocketInfo&) = delete;
        SocketInfo() = default;
        SocketInfo(SocketInfo&&) = default;
        SocketInfo& operator=(SocketInfo &&) = default;

        SocketInfo(const PeerID peerID, const std::shared_ptr<Socket>& socketPtr)
            : peerID(peerID), socketPtr(socketPtr) {}

        PeerID peerID;
        std::shared_ptr<Socket> socketPtr;
    };

    struct RemovePeerRequest {
        RemovePeerRequest(const RemovePeerRequest& other) = delete;
        RemovePeerRequest& operator=(const RemovePeerRequest&) = delete;
        RemovePeerRequest() = default;
        RemovePeerRequest(RemovePeerRequest&&) = default;
        RemovePeerRequest& operator=(RemovePeerRequest &&) = default;

        RemovePeerRequest(const PeerID peerID,
                          const std::shared_ptr<std::condition_variable>& conditionPtr)
            : peerID(peerID), conditionPtr(conditionPtr) {}

        PeerID peerID;
        std::shared_ptr<std::condition_variable> conditionPtr;
    };

    enum class Event : int {
        FINISH,     // Shutdown request
        CALL,       // New method call in the queue
        ADD_PEER,   // New peer in the queue
        REMOVE_PEER // Remove peer
    };
    EventQueue<Event> mEventQueue;


    bool mIsRunning;

    // Mutex for the Calls queue and the map of methods.
    std::mutex mCallsMutex;
    CallQueue mCalls;
    std::unordered_map<MethodID, std::shared_ptr<MethodHandlers>> mMethodsCallbacks;
    std::unordered_map<MethodID, std::shared_ptr<SignalHandlers>> mSignalsCallbacks;
    std::unordered_map<MethodID, std::list<PeerID>> mSignalsPeers;

    // Mutex for changing mSockets map.
    // Shouldn't be locked on any read/write, that could block. Just copy the ptr.
    std::mutex mSocketsMutex;
    std::unordered_map<PeerID, std::shared_ptr<Socket> > mSockets;
    std::queue<SocketInfo> mNewSockets;
    std::queue<RemovePeerRequest> mPeersToDelete;

    // Mutex for modifying the map with return callbacks
    std::mutex mReturnCallbacksMutex;
    std::unordered_map<MessageID, ReturnCallbacks> mReturnCallbacks;

    // Mutex for setting callbacks
    std::mutex mCallbacksMutex;
    PeerCallback mNewPeerCallback;
    PeerCallback mRemovedPeerCallback;

    unsigned int mMaxNumberOfPeers;

    std::thread mThread;
    std::vector<struct pollfd> mFDs;

    std::atomic<PeerID> mPeerIDCounter;

    template<typename SentDataType, typename ReceivedDataType>
    void addMethodHandlerInternal(const MethodID methodID,
                                  const typename MethodHandler<SentDataType, ReceivedDataType>::type& process);

    template<typename SentDataType, typename ReceivedDataType>
    MessageID callInternal(const MethodID methodID,
                           const PeerID peerID,
                           const std::shared_ptr<SentDataType>& data,
                           const typename ResultHandler<ReceivedDataType>::type& process);

    template<typename ReceivedDataType>
    static void discardResultHandler(Status, std::shared_ptr<ReceivedDataType>&) {}

    void run();
    bool handleEvent();
    bool onCall();
    bool onNewPeer();
    bool onRemovePeer();
    bool handleLostConnections();
    bool handleInputs();
    bool handleInput(const PeerID peerID, const Socket& socket);
    bool onReturnValue(const PeerID peerID,
                       const Socket& socket,
                       const MessageID messageID);
    bool onRemoteCall(const PeerID peerID,
                      const Socket& socket,
                      const MethodID methodID,
                      const MessageID messageID,
                      std::shared_ptr<MethodHandlers> methodCallbacks);
    bool onRemoteSignal(const PeerID peerID,
                        const Socket& socket,
                        const MethodID methodID,
                        const MessageID messageID,
                        std::shared_ptr<SignalHandlers> signalCallbacks);
    void resetPolling();
    PeerID getNextPeerID();
    CallQueue::Call getCall();
    void removePeerInternal(const PeerID peerID, Status status);

    std::shared_ptr<EmptyData> onNewSignals(const PeerID peerID,
                                            std::shared_ptr<RegisterSignalsMessage>& data);


    void cleanCommunication();
};

template<typename SentDataType, typename ReceivedDataType>
void Processor::addMethodHandlerInternal(const MethodID methodID,
                                         const typename MethodHandler<SentDataType, ReceivedDataType>::type& method)
{
    MethodHandlers methodCall;

    methodCall.parse = [](const int fd)->std::shared_ptr<void> {
        std::shared_ptr<ReceivedDataType> data(new ReceivedDataType());
        config::loadFromFD<ReceivedDataType>(fd, *data);
        return data;
    };

    methodCall.serialize = [](const int fd, std::shared_ptr<void>& data)->void {
        config::saveToFD<SentDataType>(fd, *std::static_pointer_cast<SentDataType>(data));
    };

    methodCall.method = [method](const PeerID peerID, std::shared_ptr<void>& data)->std::shared_ptr<void> {
        std::shared_ptr<ReceivedDataType> tmpData = std::static_pointer_cast<ReceivedDataType>(data);
        return method(peerID, tmpData);
    };

    {
        Lock lock(mCallsMutex);
        mMethodsCallbacks[methodID] = std::make_shared<MethodHandlers>(std::move(methodCall));
    }
}

template<typename SentDataType, typename ReceivedDataType>
void Processor::addMethodHandler(const MethodID methodID,
                                 const typename MethodHandler<SentDataType, ReceivedDataType>::type& method)
{
    if (methodID == RETURN_METHOD_ID || methodID == REGISTER_SIGNAL_METHOD_ID) {
        LOGE("Forbidden methodID: " << methodID);
        throw IPCException("Forbidden methodID: " + std::to_string(methodID));
    }

    {
        Lock lock(mCallsMutex);
        if (mSignalsCallbacks.count(methodID)) {
            LOGE("MethodID used by a signal: " << methodID);
            throw IPCException("MethodID used by a signal: " + std::to_string(methodID));
        }
    }

    addMethodHandlerInternal<SentDataType, ReceivedDataType >(methodID, method);
}

template<typename ReceivedDataType>
void Processor::addSignalHandler(const MethodID methodID,
                                 const typename SignalHandler<ReceivedDataType>::type& handler)
{
    if (methodID == RETURN_METHOD_ID || methodID == REGISTER_SIGNAL_METHOD_ID) {
        LOGE("Forbidden methodID: " << methodID);
        throw IPCException("Forbidden methodID: " + std::to_string(methodID));
    }

    {
        Lock lock(mCallsMutex);
        if (mMethodsCallbacks.count(methodID)) {
            LOGE("MethodID used by a method: " << methodID);
            throw IPCException("MethodID used by a method: " + std::to_string(methodID));
        }
    }

    SignalHandlers signalCall;

    signalCall.parse = [](const int fd)->std::shared_ptr<void> {
        std::shared_ptr<ReceivedDataType> data(new ReceivedDataType());
        config::loadFromFD<ReceivedDataType>(fd, *data);
        return data;
    };

    signalCall.signal = [handler](const PeerID peerID, std::shared_ptr<void>& data) {
        std::shared_ptr<ReceivedDataType> tmpData = std::static_pointer_cast<ReceivedDataType>(data);
        handler(peerID, tmpData);
    };

    {
        Lock lock(mCallsMutex);
        mSignalsCallbacks[methodID] = std::make_shared<SignalHandlers>(std::move(signalCall));
    }

    if (isStarted()) {
        // Broadcast the new signal to peers
        std::vector<MethodID> ids {methodID};
        auto data = std::make_shared<RegisterSignalsMessage>(ids);

        std::list<PeerID> peersIDs;
        {
            Lock lock(mSocketsMutex);
            for (const auto kv : mSockets) {
                peersIDs.push_back(kv.first);
            }
        }

        for (const PeerID peerID : peersIDs) {
            callSync<RegisterSignalsMessage, EmptyData>(REGISTER_SIGNAL_METHOD_ID,
                                                        peerID,
                                                        data,
                                                        DEFAULT_METHOD_TIMEOUT);
        }
    }
}

template<typename SentDataType, typename ReceivedDataType>
MessageID Processor::callInternal(const MethodID methodID,
                                  const PeerID peerID,
                                  const std::shared_ptr<SentDataType>& data,
                                  const typename ResultHandler<ReceivedDataType>::type& process)
{
    Lock lock(mCallsMutex);
    MessageID messageID = mCalls.push<SentDataType, ReceivedDataType>(methodID, peerID, data, process);
    mEventQueue.send(Event::CALL);

    return messageID;
}

template<typename SentDataType, typename ReceivedDataType>
MessageID Processor::callAsync(const MethodID methodID,
                               const PeerID peerID,
                               const std::shared_ptr<SentDataType>& data,
                               const typename ResultHandler<ReceivedDataType>::type& process)
{
    if (!isStarted()) {
        LOGE("The Processor thread is not started. Can't send any data.");
        throw IPCException("The Processor thread is not started. Can't send any data.");
    }

    return callInternal<SentDataType, ReceivedDataType>(methodID, peerID, data, process);
}


template<typename SentDataType, typename ReceivedDataType>
std::shared_ptr<ReceivedDataType> Processor::callSync(const MethodID methodID,
                                                      const PeerID peerID,
                                                      const std::shared_ptr<SentDataType>& data,
                                                      unsigned int timeoutMS)
{
    std::shared_ptr<ReceivedDataType> result;

    std::mutex mutex;
    std::condition_variable cv;
    Status returnStatus = ipc::Status::UNDEFINED;

    auto process = [&result, &mutex, &cv, &returnStatus](Status status, std::shared_ptr<ReceivedDataType> returnedData) {
        std::unique_lock<std::mutex> lock(mutex);
        returnStatus = status;
        result = returnedData;
        cv.notify_all();
    };

    MessageID messageID = callAsync<SentDataType, ReceivedDataType>(methodID,
                                                                    peerID,
                                                                    data,
                                                                    process);

    auto isResultInitialized = [&returnStatus]() {
        return returnStatus != ipc::Status::UNDEFINED;
    };

    std::unique_lock<std::mutex> lock(mutex);
    if (!cv.wait_for(lock, std::chrono::milliseconds(timeoutMS), isResultInitialized)) {
        bool isTimeout = false;
        {
            Lock lock(mReturnCallbacksMutex);
            if (1 == mReturnCallbacks.erase(messageID)) {
                isTimeout = true;
            }
        }
        if (isTimeout) {
            removePeer(peerID);
            LOGE("Function call timeout; methodID: " << methodID);
            throw IPCTimeoutException("Function call timeout; methodID: " + std::to_string(methodID));
        } else {
            // Timeout started during the return value processing, so wait for it to finish
            cv.wait(lock, isResultInitialized);
        }
    }

    throwOnError(returnStatus);

    return result;
}

template<typename SentDataType>
void Processor::signal(const MethodID methodID,
                       const std::shared_ptr<SentDataType>& data)
{
    if (!isStarted()) {
        LOGE("The Processor thread is not started. Can't send any data.");
        throw IPCException("The Processor thread is not started. Can't send any data.");
    }

    std::list<PeerID> peersIDs;
    {
        Lock lock(mSocketsMutex);
        peersIDs = mSignalsPeers[methodID];
    }

    for (const PeerID peerID : peersIDs) {
        Lock lock(mCallsMutex);
        mCalls.push<SentDataType>(methodID, peerID, data);
        mEventQueue.send(Event::CALL);
    }
}


} // namespace ipc
} // namespace security_containers

#endif // COMMON_IPC_INTERNALS_PROCESSOR_HPP
