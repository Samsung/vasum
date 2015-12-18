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

#ifndef CARGO_IPC_INTERNALS_PROCESSOR_HPP
#define CARGO_IPC_INTERNALS_PROCESSOR_HPP

#include "cargo-ipc/internals/result-builder.hpp"
#include "cargo-ipc/internals/socket.hpp"
#include "cargo-ipc/internals/request-queue.hpp"
#include "cargo-ipc/internals/method-request.hpp"
#include "cargo-ipc/internals/signal-request.hpp"
#include "cargo-ipc/internals/add-peer-request.hpp"
#include "cargo-ipc/internals/remove-peer-request.hpp"
#include "cargo-ipc/internals/send-result-request.hpp"
#include "cargo-ipc/internals/remove-method-request.hpp"
#include "cargo-ipc/internals/finish-request.hpp"
#include "cargo-ipc/epoll/event-poll.hpp"
#include "cargo-ipc/exception.hpp"
#include "cargo-ipc/method-result.hpp"
#include "cargo-ipc/types.hpp"
#include "cargo-fd/cargo-fd.hpp"
#include "cargo/fields.hpp"
#include "logger/logger.hpp"
#include "logger/logger-scope.hpp"

#include <ostream>
#include <condition_variable>
#include <mutex>
#include <chrono>
#include <vector>
#include <thread>
#include <string>
#include <list>
#include <functional>
#include <unordered_map>
#include <utility>

namespace cargo {
namespace ipc {
namespace internals {

const unsigned int DEFAULT_MAX_NUMBER_OF_PEERS = 500;
/**
* This class wraps communication via UX sockets
*
* It's intended to be used both in Client and Service classes.
* It uses a serialization mechanism from Config.
* Library user will only have to pass the types that each call will send and receive
*
* Message format:
* - MethodID  - probably casted enum.
*               MethodID == std::numeric_limits<MethodID>::max() is reserved for return messages
* - MessageID - unique id of a message exchange sent by this object instance. Used to identify reply messages.
* - Rest: The data written in a callback. One type per method.ReturnCallbacks
*
* TODO: API for removing signals
* TODO: Implement HandlerStore class for storing/handling handlers. This will simplify Processor.
* TODO: Implement CallbackStore class for storing/handling ReturnCallbacks. This will simplify Processor.
* TODO: Adding requests after stop() should fail
*
*/
class Processor {
private:
    enum class Event {
        FINISH,      // Shutdown request
        METHOD,      // New method call in the queue
        SIGNAL,      // New signal call in the queue
        ADD_PEER,    // New peer in the queue
        REMOVE_PEER, // Remove peer
        SEND_RESULT,  // Send the result of a method's call
        REMOVE_METHOD  // Remove method handler
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
     * @param eventPoll event poll
     * @param logName log name
     * @param newPeerCallback called when a new peer arrives
     * @param removedPeerCallback called when the Processor stops listening for this peer
     * @param maxNumberOfPeers maximal number of peers
     */
    Processor(epoll::EventPoll& eventPoll,
              const std::string& logName = "",
              const PeerCallback& newPeerCallback = nullptr,
              const PeerCallback& removedPeerCallback = nullptr,
              const unsigned int maxNumberOfPeers = DEFAULT_MAX_NUMBER_OF_PEERS);
    ~Processor();

    Processor(const Processor&) = delete;
    Processor(Processor&&) = delete;
    Processor& operator=(const Processor&) = delete;


    /**
     * Start processing.
     */
    void start();

    /**
     * @return is processor running
     */
    bool isStarted();

    /**
     * Stops the processing thread.
     * No incoming data will be handled after.
     *
     * @param wait does it block waiting for all internals to stop
     */
    void stop(bool wait);

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
     * @return peerID of the new user
     */
    PeerID addPeer(const std::shared_ptr<Socket>& socketPtr);

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
     * Send result of the method.
     * Used for asynchronous communication, only internally.
     *
     * @param methodID API dependent id of the method
     * @param peerID id of the peer
     * @param messageID id of the message to which it replies
     * @param data data to send
     */
    void sendResult(const MethodID methodID,
                    const PeerID& peerID,
                    const MessageID& messageID,
                    const std::shared_ptr<void>& data);

    /**
     * Send error result of the method
     *
     * @param peerID id of the peer
     * @param messageID id of the message to which it replies
     * @param errorCode code of the error
     * @param message description of the error
     */
    void sendError(const PeerID& peerID,
                   const MessageID& messageID,
                   const int errorCode,
                   const std::string& message);

    /**
     * Indicate that the method handler finished
     *
     * @param methodID API dependent id of the method
     * @param peerID id of the peer
     * @param messageID id of the message to which it replies
     */
    void sendVoid(const MethodID methodID,
                  const PeerID& peerID,
                  const MessageID& messageID);

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
     * @param methodID          API dependent id of the method
     * @param peerID            id of the peer
     * @param data              data to send
     * @param timeoutMS         optional, how long to wait for the return value before throw (milliseconds, default: 5000)
     * @tparam SentDataType     data type to send
     * @tparam ReceivedDataType data type to receive
     * @return call result data
     */
    template<typename SentDataType, typename ReceivedDataType>
    std::shared_ptr<ReceivedDataType> callSync(const MethodID methodID,
                                               const PeerID& peerID,
                                               const std::shared_ptr<SentDataType>& data,
                                               unsigned int timeoutMS = 5000);

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
                        const PeerID& peerID,
                        const std::shared_ptr<SentDataType>& data,
                        const typename ResultHandler<ReceivedDataType>::type& process);

    /**
     * The same as callAsync, but not blocking on the state mutex.
     *
     * @param methodID API dependent id of the method
     * @param peerID id of the peer
     * @param data data to sent
     * @param process callback processing the return data
     * @tparam SentDataType data type to send
     * @tparam ReceivedDataType data type to receive
     */
    template<typename SentDataType, typename ReceivedDataType>
    MessageID callAsyncNonBlock(const MethodID methodID,
                                const PeerID& peerID,
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
     * @param fd file description identifying the peer
     */
    void handleLostConnection(const FileDescriptor fd);

    /**
     * Handles input from one peer.
     * Handler used in external polling.
     *
     * @param fd file description identifying the peer
     */
    void handleInput(const FileDescriptor fd);

    /**
     * Handle one event from the internal event's queue
     */
    void handleEvent();

    /**
     * @return file descriptor for the internal event's queue
     */
    FileDescriptor getEventFD();

private:
    typedef std::unique_lock<std::mutex> Lock;
    typedef RequestQueue<Event>::Request Request;

    struct EmptyData {
        CARGO_REGISTER_EMPTY
    };

    struct MessageHeader {
        MethodID methodID;
        MessageID messageID;

        CARGO_REGISTER
        (
            methodID,
            messageID
        )
    };

    struct RegisterSignalsProtocolMessage {
        RegisterSignalsProtocolMessage() = default;
        explicit RegisterSignalsProtocolMessage(const std::vector<MethodID>& ids)
            : ids(ids) {}

        std::vector<MethodID> ids;

        CARGO_REGISTER
        (
            ids
        )
    };

    struct ErrorProtocolMessage {
        ErrorProtocolMessage() = default;
        ErrorProtocolMessage(const MessageID& messageID, const int code, const std::string& message)
            : messageID(messageID), code(code), message(message) {}

        MessageID messageID;
        int code;
        std::string message;

        CARGO_REGISTER
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

        ReturnCallbacks(PeerID peerID, const ParseCallback& parse, const ResultBuilderHandler& process)
            : peerID(peerID), parse(parse), process(process) {}

        PeerID peerID;
        ParseCallback parse;
        ResultBuilderHandler process;
    };

    struct PeerInfo {
        PeerInfo(const PeerInfo& other) = delete;
        PeerInfo& operator=(const PeerInfo&) = delete;
        PeerInfo() = delete;

        PeerInfo(PeerInfo&&) = default;
        PeerInfo& operator=(PeerInfo &&) = default;

        PeerInfo(PeerID peerID, const std::shared_ptr<Socket>& socketPtr)
            : peerID(peerID), socketPtr(socketPtr) {}

        PeerID peerID;
        std::shared_ptr<Socket> socketPtr;
    };

    epoll::EventPoll& mEventPoll;

    typedef std::vector<PeerInfo> Peers;

    std::string mLogPrefix;

    RequestQueue<Event> mRequestQueue;

    bool mIsRunning;

    std::unordered_map<MethodID, std::shared_ptr<MethodHandlers>> mMethodsCallbacks;
    std::unordered_map<MethodID, std::shared_ptr<SignalHandlers>> mSignalsCallbacks;
    std::unordered_map<MethodID, std::list<PeerID>> mSignalsPeers;

    Peers mPeerInfo;

    std::unordered_map<MessageID, ReturnCallbacks> mReturnCallbacks;

    // Mutex for modifying any internal data
    std::mutex mStateMutex;

    PeerCallback mNewPeerCallback;
    PeerCallback mRemovedPeerCallback;

    unsigned int mMaxNumberOfPeers;

    template<typename SentDataType, typename ReceivedDataType>
    void setMethodHandlerInternal(const MethodID methodID,
                                  const typename MethodHandler<SentDataType, ReceivedDataType>::type& process);

    template<typename ReceivedDataType>
    void setSignalHandlerInternal(const MethodID methodID,
                                  const typename SignalHandler<ReceivedDataType>::type& handler);

    template<typename SentDataType>
    void signalInternal(const MethodID methodID,
                        const PeerID& peerID,
                        const std::shared_ptr<SentDataType>& data);

    // Request handlers
    void onMethodRequest(MethodRequest& request);
    void onSignalRequest(SignalRequest& request);
    void onAddPeerRequest(AddPeerRequest& request);
    void onRemovePeerRequest(RemovePeerRequest& request);
    void onSendResultRequest(SendResultRequest& request);
    void onRemoveMethodRequest(RemoveMethodRequest& request);
    void onFinishRequest(FinishRequest& request);

    void onReturnValue(Peers::iterator& peerIt,
                       const MessageID& messageID);
    void onRemoteMethod(Peers::iterator& peerIt,
                        const MethodID methodID,
                        const MessageID& messageID,
                        std::shared_ptr<MethodHandlers> methodCallbacks);
    void onRemoteSignal(Peers::iterator& peerIt,
                        const MethodID methodID,
                        const MessageID& messageID,
                        std::shared_ptr<SignalHandlers> signalCallbacks);

    void removePeerInternal(Peers::iterator peerIt,
                            const std::exception_ptr& exceptionPtr);
    void removePeerSyncInternal(const PeerID& peerID, Lock& lock);

    HandlerExitCode onNewSignals(const PeerID& peerID,
                                 std::shared_ptr<RegisterSignalsProtocolMessage>& data);

    HandlerExitCode onErrorSignal(const PeerID& peerID,
                                  std::shared_ptr<ErrorProtocolMessage>& data);

    Peers::iterator getPeerInfoIterator(const FileDescriptor fd);
    Peers::iterator getPeerInfoIterator(const PeerID& peerID);

};

template<typename SentDataType, typename ReceivedDataType>
void Processor::setMethodHandlerInternal(const MethodID methodID,
                                         const typename MethodHandler<SentDataType, ReceivedDataType>::type& method)
{
    MethodHandlers methodCall;

    methodCall.parse = [](const int fd)->std::shared_ptr<void> {
        std::shared_ptr<ReceivedDataType> data(new ReceivedDataType());
        cargo::loadFromFD<ReceivedDataType>(fd, *data);
        return data;
    };

    methodCall.serialize = [](const int fd, std::shared_ptr<void>& data)->void {
        cargo::saveToFD<SentDataType>(fd, *std::static_pointer_cast<SentDataType>(data));
    };

    methodCall.method = [method](const PeerID peerID, std::shared_ptr<void>& data, MethodResult::Pointer && methodResult) {
        std::shared_ptr<ReceivedDataType> tmpData = std::static_pointer_cast<ReceivedDataType>(data);
        return method(peerID, tmpData, std::forward<MethodResult::Pointer>(methodResult));
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
        cargo::loadFromFD<ReceivedDataType>(fd, *dataToFill);
        return dataToFill;
    };

    signalCall.signal = [handler](const PeerID peerID, std::shared_ptr<void>& dataReceived) {
        std::shared_ptr<ReceivedDataType> tmpData = std::static_pointer_cast<ReceivedDataType>(dataReceived);
        return handler(peerID, tmpData);
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

        for (const PeerInfo& peerInfo : mPeerInfo) {
            signalInternal<RegisterSignalsProtocolMessage>(REGISTER_SIGNAL_METHOD_ID,
                                                           peerInfo.peerID,
                                                           data);
        }
    }
}


template<typename SentDataType, typename ReceivedDataType>
MessageID Processor::callAsync(const MethodID methodID,
                               const PeerID& peerID,
                               const std::shared_ptr<SentDataType>& data,
                               const typename ResultHandler<ReceivedDataType>::type& process)
{
    Lock lock(mStateMutex);
    return callAsyncNonBlock<SentDataType, ReceivedDataType>(methodID, peerID, data, process);
}

template<typename SentDataType, typename ReceivedDataType>
MessageID Processor::callAsyncNonBlock(const MethodID methodID,
                                       const PeerID& peerID,
                                       const std::shared_ptr<SentDataType>& data,
                                       const typename ResultHandler<ReceivedDataType>::type& process)
{
    auto request = MethodRequest::create<SentDataType, ReceivedDataType>(methodID, peerID, data, process);
    mRequestQueue.pushBack(Event::METHOD, request);
    return request->messageID;
}


template<typename SentDataType, typename ReceivedDataType>
std::shared_ptr<ReceivedDataType> Processor::callSync(const MethodID methodID,
                                                      const PeerID& peerID,
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

    Lock lock(mStateMutex);
    MessageID messageID = callAsyncNonBlock<SentDataType, ReceivedDataType>(methodID,
                                                                            peerID,
                                                                            data,
                                                                            process);

    auto isResultInitialized = [&result]() {
        return result.isSet();
    };

    LOGT(mLogPrefix + "Waiting for the response...");
    //In the case of too large sending time response can be received far after timeoutMS but
    //before this thread wakes up and before predicate check (there will by no timeout exception)
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
            removePeerSyncInternal(peerID, lock);
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
                               const PeerID& peerID,
                               const std::shared_ptr<SentDataType>& data)
{
    auto requestPtr = SignalRequest::create<SentDataType>(methodID, peerID, data);
    mRequestQueue.pushFront(Event::SIGNAL, requestPtr);
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
    for (const PeerID peerID : it->second) {
        auto requestPtr =  SignalRequest::create<SentDataType>(methodID, peerID, data);
        mRequestQueue.pushBack(Event::SIGNAL, requestPtr);
    }
}



} // namespace internals
} // namespace ipc
} // namespace cargo

#endif // CARGO_IPC_INTERNALS_PROCESSOR_HPP
