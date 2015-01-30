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
 * @brief   Data and event processing thread
 */

#ifndef COMMON_IPC_INTERNALS_PROCESSOR_HPP
#define COMMON_IPC_INTERNALS_PROCESSOR_HPP

#include "ipc/internals/result-builder.hpp"
#include "ipc/internals/socket.hpp"
#include "ipc/internals/request-queue.hpp"
#include "ipc/internals/method-request.hpp"
#include "ipc/internals/signal-request.hpp"
#include "ipc/internals/add-peer-request.hpp"
#include "ipc/internals/remove-peer-request.hpp"
#include "ipc/internals/finish-request.hpp"
#include "ipc/exception.hpp"
#include "ipc/types.hpp"
#include "config/manager.hpp"
#include "config/fields.hpp"
#include "logger/logger.hpp"
#include "logger/logger-scope.hpp"

#include <ostream>
#include <poll.h>
#include <condition_variable>
#include <mutex>
#include <chrono>
#include <vector>
#include <thread>
#include <string>
#include <list>
#include <functional>
#include <unordered_map>

namespace vasum {
namespace ipc {

const unsigned int DEFAULT_MAX_NUMBER_OF_PEERS = 500;

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
*  - synchronous call to many peers
*  - implement HandlerStore class for storing both signals and methods
*  - API for removing signals
*  - implement CallbackStore - thread safe calling/setting callbacks
*  - helper function for removing from unordered map
*  - new way to generate UIDs
*  - callbacks for serialization/parsing
*  - store Sockets in a vector, maybe SocketStore?
*  - poll loop outside.
*  - waiting till the EventQueue is empty before leaving stop()
*  - no new events added after stop() called
*  - when using IPCGSource: addFD and removeFD can be called from addPeer removePeer callbacks, but
*    there is no mechanism to ensure the IPCSource exists.. therefore SIGSEGV :)
*
*/
class Processor {
private:
    enum class Event {
        FINISH,     // Shutdown request
        METHOD,     // New method call in the queue
        SIGNAL,     // New signal call in the queue
        ADD_PEER,   // New peer in the queue
        REMOVE_PEER // Remove peer
    };

public:
    friend std::ostream& operator<<(std::ostream& os, const Processor::Event& event);

    /**
     * Used to indicate a message with the return value.
     */
    static const MethodID RETURN_METHOD_ID;

    /**
     * Indicates an Processor's internal request/broadcast to register a Signal
     */
    static const MethodID REGISTER_SIGNAL_METHOD_ID;

    /**
    * Error return message
    */
    static const MethodID ERROR_METHOD_ID;

    /**
     * Constructs the Processor, but doesn't start it.
     * The object is ready to add methods.
     *
     * @param newPeerCallback called when a new peer arrives
     * @param removedPeerCallback called when the Processor stops listening for this peer
     */
    Processor(const std::string& logName = "",
              const PeerCallback& newPeerCallback = nullptr,
              const PeerCallback& removedPeerCallback = nullptr,
              const unsigned int maxNumberOfPeers = DEFAULT_MAX_NUMBER_OF_PEERS);
    ~Processor();

    Processor(const Processor&) = delete;
    Processor(Processor&&) = delete;
    Processor& operator=(const Processor&) = delete;


    /**
     * Start the processing thread.
     * Quits immediately after starting the thread.
     *
     * @param usesExternalPolling internal or external polling is used
     */
    void start(const bool usesExternalPolling);

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
     * @return peerFD of the new socket
     */
    FileDescriptor addPeer(const std::shared_ptr<Socket>& socketPtr);

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
    void setMethodHandler(const MethodID methodID,
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
    void setSignalHandler(const MethodID methodID,
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
     * @param peerFD id of the peer
     * @param data data to sent
     * @param timeoutMS how long to wait for the return value before throw
     * @tparam SentDataType data type to send
     * @tparam ReceivedDataType data type to receive
     */
    template<typename SentDataType, typename ReceivedDataType>
    std::shared_ptr<ReceivedDataType> callSync(const MethodID methodID,
                                               const FileDescriptor peerFD,
                                               const std::shared_ptr<SentDataType>& data,
                                               unsigned int timeoutMS = 500);

    /**
     * Asynchronous method call
     *
     * @param methodID API dependent id of the method
     * @param peerFD id of the peer
     * @param data data to sent
     * @param process callback processing the return data
     * @tparam SentDataType data type to send
     * @tparam ReceivedDataType data type to receive
     */
    template<typename SentDataType, typename ReceivedDataType>
    MessageID callAsync(const MethodID methodID,
                        const FileDescriptor peerFD,
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

    /**
     * Removes one peer.
     * Handler used in external polling.
     *
     * @param peerFD file description identifying the peer
     * @return should the polling structure be rebuild
     */
    bool handleLostConnection(const FileDescriptor peerFD);

    /**
     * Handles input from one peer.
     * Handler used in external polling.
     *
     * @param peerFD file description identifying the peer
     * @return should the polling structure be rebuild
     */
    bool handleInput(const FileDescriptor peerFD);

    /**
     * Handle one event from the internal event's queue
     *
     * @return should the polling structure be rebuild
     */
    bool handleEvent();

    /**
     * @return file descriptor for the internal event's queue
     */
    FileDescriptor getEventFD();

private:
    typedef std::function<void(int fd, std::shared_ptr<void>& data)> SerializeCallback;
    typedef std::function<std::shared_ptr<void>(int fd)> ParseCallback;
    typedef std::unique_lock<std::mutex> Lock;
    typedef RequestQueue<Event>::Request Request;

    struct EmptyData {
        CONFIG_REGISTER_EMPTY
    };

    struct RegisterSignalsProtocolMessage {
        RegisterSignalsProtocolMessage() = default;
        RegisterSignalsProtocolMessage(const std::vector<MethodID> ids)
            : ids(ids) {}

        std::vector<MethodID> ids;

        CONFIG_REGISTER
        (
            ids
        )
    };

    struct ErrorProtocolMessage {
        ErrorProtocolMessage() = default;
        ErrorProtocolMessage(const MessageID messageID, const int code, const std::string& message)
            : messageID(messageID), code(code), message(message) {}

        MessageID messageID;
        int code;
        std::string message;

        CONFIG_REGISTER
        (
            messageID,
            code,
            message
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

        ReturnCallbacks(FileDescriptor peerFD, const ParseCallback& parse, const ResultBuilderHandler& process)
            : peerFD(peerFD), parse(parse), process(process) {}

        FileDescriptor peerFD;
        ParseCallback parse;
        ResultBuilderHandler process;
    };

    std::string mLogPrefix;

    RequestQueue<Event> mRequestQueue;

    bool mIsRunning;
    bool mUsesExternalPolling;

    std::unordered_map<MethodID, std::shared_ptr<MethodHandlers>> mMethodsCallbacks;
    std::unordered_map<MethodID, std::shared_ptr<SignalHandlers>> mSignalsCallbacks;
    std::unordered_map<MethodID, std::list<FileDescriptor>> mSignalsPeers;

    std::unordered_map<FileDescriptor, std::shared_ptr<Socket> > mSockets;
    std::vector<struct pollfd> mFDs;

    std::unordered_map<MessageID, ReturnCallbacks> mReturnCallbacks;

    // Mutex for modifying any internal data
    std::mutex mStateMutex;

    PeerCallback mNewPeerCallback;
    PeerCallback mRemovedPeerCallback;

    unsigned int mMaxNumberOfPeers;

    std::thread mThread;

    template<typename SentDataType, typename ReceivedDataType>
    void setMethodHandlerInternal(const MethodID methodID,
                                  const typename MethodHandler<SentDataType, ReceivedDataType>::type& process);

    template<typename ReceivedDataType>
    void setSignalHandlerInternal(const MethodID methodID,
                                  const typename SignalHandler<ReceivedDataType>::type& handler);

    template<typename SentDataType>
    void signalInternal(const MethodID methodID,
                        const FileDescriptor peerFD,
                        const std::shared_ptr<SentDataType>& data);

    void run();

    // Request handlers
    bool onMethodRequest(MethodRequest& request);
    bool onSignalRequest(SignalRequest& request);
    bool onAddPeerRequest(AddPeerRequest& request);
    bool onRemovePeerRequest(RemovePeerRequest& request);
    bool onFinishRequest(FinishRequest& request);

    bool handleLostConnections();
    bool handleInputs();

    bool onReturnValue(const Socket& socket,
                       const MessageID messageID);
    bool onRemoteMethod(const Socket& socket,
                        const MethodID methodID,
                        const MessageID messageID,
                        std::shared_ptr<MethodHandlers> methodCallbacks);
    bool onRemoteSignal(const Socket& socket,
                        const MethodID methodID,
                        const MessageID messageID,
                        std::shared_ptr<SignalHandlers> signalCallbacks);
    void resetPolling();
    FileDescriptor getNextFileDescriptor();
    void removePeerInternal(const FileDescriptor peerFD, const std::exception_ptr& exceptionPtr);
    void removePeerSyncInternal(const FileDescriptor peerFD, Lock& lock);

    void onNewSignals(const FileDescriptor peerFD,
                      std::shared_ptr<RegisterSignalsProtocolMessage>& data);

    void onErrorSignal(const FileDescriptor peerFD,
                       std::shared_ptr<ErrorProtocolMessage>& data);


};

template<typename SentDataType, typename ReceivedDataType>
void Processor::setMethodHandlerInternal(const MethodID methodID,
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

    methodCall.method = [method](const FileDescriptor peerFD, std::shared_ptr<void>& data)->std::shared_ptr<void> {
        std::shared_ptr<ReceivedDataType> tmpData = std::static_pointer_cast<ReceivedDataType>(data);
        return method(peerFD, tmpData);
    };

    mMethodsCallbacks[methodID] = std::make_shared<MethodHandlers>(std::move(methodCall));
}

template<typename SentDataType, typename ReceivedDataType>
void Processor::setMethodHandler(const MethodID methodID,
                                 const typename MethodHandler<SentDataType, ReceivedDataType>::type& method)
{
    if (methodID == RETURN_METHOD_ID || methodID == REGISTER_SIGNAL_METHOD_ID) {
        LOGE(mLogPrefix + "Forbidden methodID: " << methodID);
        throw IPCException("Forbidden methodID: " + std::to_string(methodID));
    }

    {
        Lock lock(mStateMutex);

        if (mSignalsCallbacks.count(methodID)) {
            LOGE(mLogPrefix + "MethodID used by a signal: " << methodID);
            throw IPCException("MethodID used by a signal: " + std::to_string(methodID));
        }

        setMethodHandlerInternal<SentDataType, ReceivedDataType>(methodID, method);
    }

}

template<typename ReceivedDataType>
void Processor::setSignalHandlerInternal(const MethodID methodID,
                                         const typename SignalHandler<ReceivedDataType>::type& handler)
{
    SignalHandlers signalCall;

    signalCall.parse = [](const int fd)->std::shared_ptr<void> {
        std::shared_ptr<ReceivedDataType> dataToFill(new ReceivedDataType());
        config::loadFromFD<ReceivedDataType>(fd, *dataToFill);
        return dataToFill;
    };

    signalCall.signal = [handler](const FileDescriptor peerFD, std::shared_ptr<void>& dataReceived) {
        std::shared_ptr<ReceivedDataType> tmpData = std::static_pointer_cast<ReceivedDataType>(dataReceived);
        handler(peerFD, tmpData);
    };

    mSignalsCallbacks[methodID] = std::make_shared<SignalHandlers>(std::move(signalCall));
}


template<typename ReceivedDataType>
void Processor::setSignalHandler(const MethodID methodID,
                                 const typename SignalHandler<ReceivedDataType>::type& handler)
{
    if (methodID == RETURN_METHOD_ID || methodID == REGISTER_SIGNAL_METHOD_ID) {
        LOGE(mLogPrefix + "Forbidden methodID: " << methodID);
        throw IPCException("Forbidden methodID: " + std::to_string(methodID));
    }

    std::shared_ptr<RegisterSignalsProtocolMessage> data;

    {
        Lock lock(mStateMutex);

        // Andd the signal handler:
        if (mMethodsCallbacks.count(methodID)) {
            LOGE(mLogPrefix + "MethodID used by a method: " << methodID);
            throw IPCException("MethodID used by a method: " + std::to_string(methodID));
        }

        setSignalHandlerInternal<ReceivedDataType>(methodID, handler);

        // Broadcast the new signal:
        std::vector<MethodID> ids {methodID};
        data = std::make_shared<RegisterSignalsProtocolMessage>(ids);

        for (const auto kv : mSockets) {
            signalInternal<RegisterSignalsProtocolMessage>(REGISTER_SIGNAL_METHOD_ID,
                                                           kv.first,
                                                           data);
        }
    }
}


template<typename SentDataType, typename ReceivedDataType>
MessageID Processor::callAsync(const MethodID methodID,
                               const FileDescriptor peerFD,
                               const std::shared_ptr<SentDataType>& data,
                               const typename ResultHandler<ReceivedDataType>::type& process)
{
    Lock lock(mStateMutex);
    auto request = MethodRequest::create<SentDataType, ReceivedDataType>(methodID, peerFD, data, process);
    mRequestQueue.pushBack(Event::METHOD, request);
    return request->messageID;
}


template<typename SentDataType, typename ReceivedDataType>
std::shared_ptr<ReceivedDataType> Processor::callSync(const MethodID methodID,
                                                      const FileDescriptor peerFD,
                                                      const std::shared_ptr<SentDataType>& data,
                                                      unsigned int timeoutMS)
{
    Result<ReceivedDataType> result;
    std::condition_variable cv;

    auto process = [&result, &cv](const Result<ReceivedDataType> && r) {
        // This is called under lock(mStateMutex)
        result = std::move(r);
        cv.notify_all();
    };

    MessageID messageID = callAsync<SentDataType, ReceivedDataType>(methodID,
                                                                    peerFD,
                                                                    data,
                                                                    process);

    auto isResultInitialized = [&result]() {
        return result.isValid();
    };

    Lock lock(mStateMutex);
    LOGT(mLogPrefix + "Waiting for the response...");
    if (!cv.wait_for(lock, std::chrono::milliseconds(timeoutMS), isResultInitialized)) {
        LOGW(mLogPrefix + "Probably a timeout in callSync. Checking...");

        // Call isn't sent or call is sent but there is no reply
        bool isTimeout = mRequestQueue.removeIf([messageID](Request & request) {
            return request.requestID == Event::METHOD &&
                   request.get<MethodRequest>()->messageID == messageID;
        })
        || 1 == mReturnCallbacks.erase(messageID);

        if (isTimeout) {
            LOGE(mLogPrefix + "Function call timeout; methodID: " << methodID);
            removePeerSyncInternal(peerFD, lock);
            throw IPCTimeoutException("Function call timeout; methodID: " + std::to_string(methodID));
        } else {
            LOGW(mLogPrefix + "Timeout started during the return value processing, so wait for it to finish");
            if (!cv.wait_for(lock, std::chrono::milliseconds(timeoutMS), isResultInitialized)) {
                LOGE(mLogPrefix + "Function call timeout; methodID: " << methodID);
                throw IPCTimeoutException("Function call timeout; methodID: " + std::to_string(methodID));
            }
        }
    }

    return result.get();
}

template<typename SentDataType>
void Processor::signalInternal(const MethodID methodID,
                               const FileDescriptor peerFD,
                               const std::shared_ptr<SentDataType>& data)
{
    auto request = SignalRequest::create<SentDataType>(methodID, peerFD, data);
    mRequestQueue.pushFront(Event::SIGNAL, request);
}

template<typename SentDataType>
void Processor::signal(const MethodID methodID,
                       const std::shared_ptr<SentDataType>& data)
{
    Lock lock(mStateMutex);
    const auto it = mSignalsPeers.find(methodID);
    if (it == mSignalsPeers.end()) {
        LOGW(mLogPrefix + "No peer is handling signal with methodID: " << methodID);
        return;
    }
    for (const FileDescriptor peerFD : it->second) {
        auto request =  SignalRequest::create<SentDataType>(methodID, peerFD, data);
        mRequestQueue.pushBack(Event::SIGNAL, request);
    }
}



} // namespace ipc
} // namespace vasum

#endif // COMMON_IPC_INTERNALS_PROCESSOR_HPP
