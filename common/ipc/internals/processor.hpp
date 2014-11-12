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
#include "ipc/exception.hpp"
#include "ipc/types.hpp"
#include "config/manager.hpp"
#include "config/is-visitable.hpp"
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
#include <functional>
#include <unordered_map>

namespace security_containers {
namespace ipc {
namespace {
const unsigned int DEFAULT_MAX_NUMBER_OF_PEERS = 500;
}
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
*  - error codes passed to async callbacks
*  - remove ReturnCallbacks on peer disconnect
*  - on sync timeout erase the return callback
*  - don't throw timeout if the message is already processed
*  - naming convention or methods that just commissions the PROCESS thread to do something
*  - removePeer API function
*  - error handling - special message type
*  - some mutexes may not be needed
*/
class Processor {
public:
    typedef std::function<void(int)> PeerCallback;
    typedef unsigned int PeerID;
    typedef unsigned int MethodID;

    /**
     * Method ID. Used to indicate a message with the return value.
     */
    static const MethodID RETURN_METHOD_ID;
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
     * Stops the processing thread.
     * No incoming data will be handled after.
     */
    void stop();

    /**
     * From now on socket is owned by the Processor object.
     * Calls the newPeerCallback.
     *
     * @param socketPtr pointer to the new socket
     * @return peerID of the new socket
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
    void addMethodHandler(const MethodID methodID,
                          const typename MethodHandler<SentDataType, ReceivedDataType>::type& process);

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
    void callAsync(const MethodID methodID,
                   const PeerID peerID,
                   const std::shared_ptr<SentDataType>& data,
                   const typename ResultHandler<ReceivedDataType>::type& process);


private:
    typedef std::function<void(int fd, std::shared_ptr<void>& data)> SerializeCallback;
    typedef std::function<std::shared_ptr<void>(int fd)> ParseCallback;
    typedef std::lock_guard<std::mutex> Lock;
    typedef unsigned int MessageID;

    struct Call {
        Call(const Call& other) = delete;
        Call& operator=(const Call&) = delete;
        Call() = default;
        Call(Call&&) = default;

        PeerID peerID;
        MethodID methodID;
        std::shared_ptr<void> data;
        SerializeCallback serialize;
        ParseCallback parse;
        ResultHandler<void>::type process;
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

    enum class Event : int {
        FINISH,     // Shutdown request
        CALL,       // New method call in the queue
        NEW_PEER    // New peer in the queue
    };
    EventQueue<Event> mEventQueue;


    bool mIsRunning;

    // Mutex for the Calls queue and the map of methods.
    std::mutex mCallsMutex;
    std::queue<Call> mCalls;
    std::unordered_map<MethodID, std::shared_ptr<MethodHandlers>> mMethodsCallbacks;

    // Mutex for changing mSockets map.
    // Shouldn't be locked on any read/write, that could block. Just copy the ptr.
    std::mutex mSocketsMutex;
    std::unordered_map<PeerID, std::shared_ptr<Socket> > mSockets;
    std::queue<SocketInfo> mNewSockets;

    // Mutex for modifying the map with return callbacks
    std::mutex mReturnCallbacksMutex;
    std::unordered_map<MessageID, ReturnCallbacks> mReturnCallbacks;


    PeerCallback mNewPeerCallback;
    PeerCallback mRemovedPeerCallback;

    unsigned int mMaxNumberOfPeers;

    std::thread mThread;
    std::vector<struct pollfd> mFDs;

    std::atomic<MessageID> mMessageIDCounter;
    std::atomic<PeerID> mPeerIDCounter;

    void run();
    bool handleEvent();
    bool handleCall();
    bool handleLostConnections();
    bool handleInputs();
    bool handleInput(const PeerID peerID, const Socket& socket);
    bool onReturnValue(const PeerID peerID,
                       const Socket& socket,
                       const MessageID messageID);
    bool onRemoteCall(const PeerID peerID,
                      const Socket& socket,
                      const MethodID methodID,
                      const MessageID messageID);
    void resetPolling();
    MessageID getNextMessageID();
    PeerID getNextPeerID();
    Call getCall();
    void removePeer(const PeerID peerID, Status status);

};

template<typename SentDataType, typename ReceivedDataType>
void Processor::addMethodHandler(const MethodID methodID,
                                 const typename MethodHandler<SentDataType, ReceivedDataType>::type& method)
{
    static_assert(config::isVisitable<SentDataType>::value,
                  "Use the libConfig library");
    static_assert(config::isVisitable<ReceivedDataType>::value,
                  "Use the libConfig library");

    if (methodID == RETURN_METHOD_ID) {
        LOGE("Forbidden methodID: " << methodID);
        throw IPCException("Forbidden methodID: " + std::to_string(methodID));
    }

    using namespace std::placeholders;

    MethodHandlers methodCall;

    methodCall.parse = [](const int fd)->std::shared_ptr<void> {
        std::shared_ptr<ReceivedDataType> data(new ReceivedDataType());
        config::loadFromFD<ReceivedDataType>(fd, *data);
        return data;
    };

    methodCall.serialize = [](const int fd, std::shared_ptr<void>& data)->void {
        config::saveToFD<SentDataType>(fd, *std::static_pointer_cast<SentDataType>(data));
    };

    methodCall.method = [method](std::shared_ptr<void>& data)->std::shared_ptr<void> {
        std::shared_ptr<ReceivedDataType> tmpData = std::static_pointer_cast<ReceivedDataType>(data);
        return method(tmpData);
    };

    {
        Lock lock(mCallsMutex);
        mMethodsCallbacks[methodID] = std::make_shared<MethodHandlers>(std::move(methodCall));
    }
}

template<typename SentDataType, typename ReceivedDataType>
void Processor::callAsync(const MethodID methodID,
                          const PeerID peerID,
                          const std::shared_ptr<SentDataType>& data,
                          const typename  ResultHandler<ReceivedDataType>::type& process)
{
    static_assert(config::isVisitable<SentDataType>::value,
                  "Use the libConfig library");
    static_assert(config::isVisitable<ReceivedDataType>::value,
                  "Use the libConfig library");

    if (!mThread.joinable()) {
        LOGE("The Processor thread is not started. Can't send any data.");
        throw IPCException("The Processor thread is not started. Can't send any data.");
    }

    using namespace std::placeholders;

    Call call;
    call.peerID = peerID;
    call.methodID = methodID;
    call.data = data;

    call.parse = [](const int fd)->std::shared_ptr<void> {
        std::shared_ptr<ReceivedDataType> data(new ReceivedDataType());
        config::loadFromFD<ReceivedDataType>(fd, *data);
        return data;
    };

    call.serialize = [](const int fd, std::shared_ptr<void>& data)->void {
        config::saveToFD<SentDataType>(fd, *std::static_pointer_cast<SentDataType>(data));
    };

    call.process = [process](Status status, std::shared_ptr<void>& data)->void {
        std::shared_ptr<ReceivedDataType> tmpData = std::static_pointer_cast<ReceivedDataType>(data);
        return process(status, tmpData);
    };

    {
        Lock lock(mCallsMutex);
        mCalls.push(std::move(call));
    }

    mEventQueue.send(Event::CALL);
}


template<typename SentDataType, typename ReceivedDataType>
std::shared_ptr<ReceivedDataType> Processor::callSync(const MethodID methodID,
                                                      const PeerID peerID,
                                                      const std::shared_ptr<SentDataType>& data,
                                                      unsigned int timeoutMS)
{
    static_assert(config::isVisitable<SentDataType>::value,
                  "Use the libConfig library");
    static_assert(config::isVisitable<ReceivedDataType>::value,
                  "Use the libConfig library");

    if (!mThread.joinable()) {
        LOGE("The Processor thread is not started. Can't send any data.");
        throw IPCException("The Processor thread is not started. Can't send any data.");
    }

    std::shared_ptr<ReceivedDataType> result;

    std::mutex mtx;
    std::unique_lock<std::mutex> lck(mtx);
    std::condition_variable cv;
    Status returnStatus = ipc::Status::UNDEFINED;

    auto process = [&result, &cv, &returnStatus](Status status, std::shared_ptr<ReceivedDataType> returnedData) {
        returnStatus = status;
        result = returnedData;
        cv.notify_one();
    };

    callAsync<SentDataType,
              ReceivedDataType>(methodID,
                                peerID,
                                data,
                                process);

    auto isResultInitialized = [&returnStatus]() {
        return returnStatus != ipc::Status::UNDEFINED;
    };

    if (!cv.wait_for(lck, std::chrono::milliseconds(timeoutMS), isResultInitialized)) {
        LOGE("Function call timeout; methodID: " << methodID);
        throw IPCTimeoutException("Function call timeout; methodID: " + std::to_string(methodID));
    }

    throwOnError(returnStatus);

    return result;
}


} // namespace ipc
} // namespace security_containers

#endif // COMMON_IPC_INTERNALS_PROCESSOR_HPP
