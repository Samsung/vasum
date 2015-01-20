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

#include "config.hpp"

#include "ipc/exception.hpp"
#include "ipc/internals/processor.hpp"
#include "ipc/internals/utils.hpp"
#include "utils/signal.hpp"

#include <cerrno>
#include <cstring>
#include <csignal>
#include <stdexcept>

#include <sys/socket.h>
#include <limits>

namespace vasum {
namespace ipc {

#define IGNORE_EXCEPTIONS(expr)                        \
    try                                                \
    {                                                  \
        expr;                                          \
    }                                                  \
    catch (const std::exception& e){                   \
        LOGE(mLogPrefix + "Callback threw an error: " << e.what()); \
    }

const MethodID Processor::RETURN_METHOD_ID = std::numeric_limits<MethodID>::max();
const MethodID Processor::REGISTER_SIGNAL_METHOD_ID = std::numeric_limits<MethodID>::max() - 1;

Processor::Processor(const std::string& logName,
                     const PeerCallback& newPeerCallback,
                     const PeerCallback& removedPeerCallback,
                     const unsigned int maxNumberOfPeers)
    : mLogPrefix(logName),
      mIsRunning(false),
      mNewPeerCallback(newPeerCallback),
      mRemovedPeerCallback(removedPeerCallback),
      mMaxNumberOfPeers(maxNumberOfPeers)
{
    LOGS(mLogPrefix + "Processor Constructor");

    utils::signalBlock(SIGPIPE);
    using namespace std::placeholders;
    setMethodHandlerInternal<EmptyData, RegisterSignalsMessage>(REGISTER_SIGNAL_METHOD_ID,
                                                                std::bind(&Processor::onNewSignals, this, _1, _2));
}

Processor::~Processor()
{
    LOGS(mLogPrefix + "Processor Destructor");
    try {
        stop();
    } catch (IPCException& e) {
        LOGE(mLogPrefix + "Error in Processor's destructor: " << e.what());
    }
}

bool Processor::isStarted()
{
    Lock lock(mStateMutex);
    return mIsRunning;
}

void Processor::start(bool usesExternalPolling)
{
    LOGS(mLogPrefix + "Processor start");

    Lock lock(mStateMutex);
    if (!isStarted()) {
        LOGI(mLogPrefix + "Processor start");
        mIsRunning = true;
        mUsesExternalPolling = usesExternalPolling;
        if (!usesExternalPolling) {
            mThread = std::thread(&Processor::run, this);
        }
    }
}

void Processor::stop()
{
    LOGS(mLogPrefix + "Processor stop");

    if (isStarted()) {
        auto conditionPtr = std::make_shared<std::condition_variable_any>();
        {
            Lock lock(mStateMutex);
            auto request = std::make_shared<FinishRequest>(conditionPtr);
            mRequestQueue.push(Event::FINISH, request);
        }

        LOGD(mLogPrefix + "Waiting for the Processor to stop");

        if (mThread.joinable()) {
            mThread.join();
        } else {
            // Wait till the FINISH request is served
            Lock lock(mStateMutex);
            conditionPtr->wait(lock, [this]() {
                return !isStarted();
            });
        }
    }
}

void Processor::setNewPeerCallback(const PeerCallback& newPeerCallback)
{
    Lock lock(mStateMutex);
    mNewPeerCallback = newPeerCallback;
}

void Processor::setRemovedPeerCallback(const PeerCallback& removedPeerCallback)
{
    Lock lock(mStateMutex);
    mRemovedPeerCallback = removedPeerCallback;
}

FileDescriptor Processor::getEventFD()
{
    Lock lock(mStateMutex);
    return mRequestQueue.getFD();
}

void Processor::removeMethod(const MethodID methodID)
{
    Lock lock(mStateMutex);
    mMethodsCallbacks.erase(methodID);
}

FileDescriptor Processor::addPeer(const std::shared_ptr<Socket>& socketPtr)
{
    LOGS(mLogPrefix + "Processor addPeer");
    Lock lock(mStateMutex);

    FileDescriptor peerFD = socketPtr->getFD();
    auto request = std::make_shared<AddPeerRequest>(peerFD, socketPtr);
    mRequestQueue.push(Event::ADD_PEER, request);

    LOGI(mLogPrefix + "Add Peer Request. Id: " << peerFD);

    return peerFD;
}

void Processor::removePeer(const FileDescriptor peerFD)
{
    LOGS(mLogPrefix + "Processor removePeer peerFD: " << peerFD);

    {
        Lock lock(mStateMutex);
        mRequestQueue.removeIf([peerFD](Request & request) {
            return request.requestID == Event::ADD_PEER &&
                   request.get<AddPeerRequest>()->peerFD == peerFD;
        });
    }

    // Remove peer and wait till he's gone
    std::shared_ptr<std::condition_variable_any> conditionPtr(new std::condition_variable_any());
    {
        Lock lock(mStateMutex);
        auto request = std::make_shared<RemovePeerRequest>(peerFD, conditionPtr);
        mRequestQueue.push(Event::REMOVE_PEER, request);
    }

    auto isPeerDeleted = [&peerFD, this]()->bool {
        return mSockets.count(peerFD) == 0;
    };

    Lock lock(mStateMutex);
    conditionPtr->wait(lock, isPeerDeleted);
}

void Processor::removePeerInternal(const FileDescriptor peerFD, Status status)
{
    LOGS(mLogPrefix + "Processor removePeerInternal peerFD: " << peerFD);
    LOGI(mLogPrefix + "Removing peer. peerFD: " << peerFD);

    if (!mSockets.erase(peerFD)) {
        LOGW(mLogPrefix + "No such peer. Another thread called removePeerInternal");
        return;
    }

    // Remove from signal addressees
    for (auto it = mSignalsPeers.begin(); it != mSignalsPeers.end();) {
        it->second.remove(peerFD);
        if (it->second.empty()) {
            it = mSignalsPeers.erase(it);
        } else {
            ++it;
        }
    }

    // Erase associated return value callbacks
    std::shared_ptr<void> data;
    for (auto it = mReturnCallbacks.begin(); it != mReturnCallbacks.end();) {
        if (it->second.peerFD == peerFD) {
            IGNORE_EXCEPTIONS(it->second.process(status, data));
            it = mReturnCallbacks.erase(it);
        } else {
            ++it;
        }
    }

    if (mRemovedPeerCallback) {
        // Notify about the deletion
        mRemovedPeerCallback(peerFD);
    }

    resetPolling();
}

void Processor::resetPolling()
{
    LOGS(mLogPrefix + "Processor resetPolling");

    if (mUsesExternalPolling) {
        return;
    }

    {
        Lock lock(mStateMutex);
        LOGI(mLogPrefix + "Reseting mFDS.size: " << mSockets.size());
        // Setup polling on eventfd and sockets
        mFDs.resize(mSockets.size() + 1);

        mFDs[0].fd = mRequestQueue.getFD();
        mFDs[0].events = POLLIN;

        auto socketIt = mSockets.begin();
        for (unsigned int i = 1; i < mFDs.size(); ++i) {
            LOGI(mLogPrefix + "Reseting fd: " << socketIt->second->getFD());
            mFDs[i].fd = socketIt->second->getFD();
            mFDs[i].events = POLLIN | POLLHUP; // Listen for input events
            ++socketIt;
            // TODO: It's possible to block on writing to fd. Maybe listen for POLLOUT too?
        }
    }
}

void Processor::run()
{
    LOGS(mLogPrefix + "Processor run");

    resetPolling();

    while (isStarted()) {
        LOGT(mLogPrefix + "Waiting for communication...");
        int ret = poll(mFDs.data(), mFDs.size(), -1 /*blocking call*/);
        LOGT(mLogPrefix + "... incoming communication!");
        if (ret == -1 || ret == 0) {
            if (errno == EINTR) {
                continue;
            }
            LOGE(mLogPrefix + "Error in poll: " << std::string(strerror(errno)));
            throw IPCException("Error in poll: " + std::string(strerror(errno)));
        }

        // Check for lost connections:
        if (handleLostConnections()) {
            // mFDs changed
            continue;
        }

        // Check for incoming data.
        if (handleInputs()) {
            // mFDs changed
            continue;
        }

        // Check for incoming events
        if (mFDs[0].revents & POLLIN) {
            mFDs[0].revents &= ~(POLLIN);
            if (handleEvent()) {
                // mFDs changed
                continue;
            }
        }

    }
}

bool Processor::handleLostConnections()
{
    Lock lock(mStateMutex);

    bool isPeerRemoved = false;
    {
        for (unsigned int i = 1; i < mFDs.size(); ++i) {
            if (mFDs[i].revents & POLLHUP) {
                LOGI(mLogPrefix + "Lost connection to peer: " << mFDs[i].fd);
                mFDs[i].revents &= ~(POLLHUP);
                removePeerInternal(mFDs[i].fd, Status::PEER_DISCONNECTED);
                isPeerRemoved = true;
            }
        }
    }

    return isPeerRemoved;
}

bool Processor::handleLostConnection(const FileDescriptor peerFD)
{
    Lock lock(mStateMutex);
    removePeerInternal(peerFD, Status::PEER_DISCONNECTED);
    return true;
}

bool Processor::handleInputs()
{
    Lock lock(mStateMutex);

    bool pollChanged = false;
    for (unsigned int i = 1; i < mFDs.size(); ++i) {
        if (mFDs[i].revents & POLLIN) {
            mFDs[i].revents &= ~(POLLIN);
            pollChanged = pollChanged || handleInput(mFDs[i].fd);
        }
    }

    return pollChanged;
}

bool Processor::handleInput(const FileDescriptor peerFD)
{
    LOGS(mLogPrefix + "Processor handleInput peerFD: " << peerFD);
    Lock lock(mStateMutex);

    std::shared_ptr<Socket> socketPtr;
    try {
        // Get the peer's socket
        socketPtr = mSockets.at(peerFD);
    } catch (const std::out_of_range&) {
        LOGE(mLogPrefix + "No such peer: " << peerFD);
        return false;
    }

    MethodID methodID;
    MessageID messageID;
    {
        Socket::Guard guard = socketPtr->getGuard();
        try {
            socketPtr->read(&methodID, sizeof(methodID));
            socketPtr->read(&messageID, sizeof(messageID));

        } catch (const IPCException& e) {
            LOGE(mLogPrefix + "Error during reading the socket");
            removePeerInternal(socketPtr->getFD(), Status::NAUGHTY_PEER);
            return true;
        }

        if (methodID == RETURN_METHOD_ID) {
            return onReturnValue(*socketPtr, messageID);

        } else {
            if (mMethodsCallbacks.count(methodID)) {
                // Method
                std::shared_ptr<MethodHandlers> methodCallbacks = mMethodsCallbacks.at(methodID);
                return onRemoteCall(*socketPtr, methodID, messageID, methodCallbacks);

            } else if (mSignalsCallbacks.count(methodID)) {
                // Signal
                std::shared_ptr<SignalHandlers> signalCallbacks = mSignalsCallbacks.at(methodID);
                return onRemoteSignal(*socketPtr, methodID, messageID, signalCallbacks);

            } else {
                // Nothing
                LOGW(mLogPrefix + "No method or signal callback for methodID: " << methodID);
                removePeerInternal(socketPtr->getFD(), Status::NAUGHTY_PEER);
                return true;
            }
        }
    }
}

std::shared_ptr<Processor::EmptyData> Processor::onNewSignals(const FileDescriptor peerFD,
                                                              std::shared_ptr<RegisterSignalsMessage>& data)
{
    LOGS(mLogPrefix + "Processor onNewSignals peerFD: " << peerFD);

    for (const MethodID methodID : data->ids) {
        mSignalsPeers[methodID].push_back(peerFD);
    }

    return std::make_shared<EmptyData>();
}

bool Processor::onReturnValue(const Socket& socket,
                              const MessageID messageID)
{
    LOGS(mLogPrefix + "Processor onReturnValue messageID: " << messageID);

    // LOGI(mLogPrefix + "Return value for messageID: " << messageID);
    ReturnCallbacks returnCallbacks;
    try {
        LOGT(mLogPrefix + "Getting the return callback");
        returnCallbacks = std::move(mReturnCallbacks.at(messageID));
        mReturnCallbacks.erase(messageID);
    } catch (const std::out_of_range&) {
        LOGW(mLogPrefix + "No return callback for messageID: " << messageID);
        removePeerInternal(socket.getFD(), Status::NAUGHTY_PEER);
        return true;
    }

    std::shared_ptr<void> data;
    try {
        LOGT(mLogPrefix + "Parsing incoming return data");
        data = returnCallbacks.parse(socket.getFD());
    } catch (const std::exception& e) {
        LOGE(mLogPrefix + "Exception during parsing: " << e.what());
        IGNORE_EXCEPTIONS(returnCallbacks.process(Status::PARSING_ERROR, data));
        removePeerInternal(socket.getFD(), Status::PARSING_ERROR);
        return true;
    }

    // LOGT(mLogPrefix + "Process return value callback for messageID: " << messageID);
    IGNORE_EXCEPTIONS(returnCallbacks.process(Status::OK, data));

    // LOGT(mLogPrefix + "Return value for messageID: " << messageID << " processed");
    return false;
}

bool Processor::onRemoteSignal(const Socket& socket,
                               const MethodID methodID,
                               const MessageID messageID,
                               std::shared_ptr<SignalHandlers> signalCallbacks)
{
    LOGS(mLogPrefix + "Processor onRemoteSignal; methodID: " << methodID << " messageID: " << messageID);

    // LOGI(mLogPrefix + "Processor onRemoteSignal; methodID: " << methodID << " messageID: " << messageID);

    std::shared_ptr<void> data;
    try {
        LOGT(mLogPrefix + "Parsing incoming data");
        data = signalCallbacks->parse(socket.getFD());
    } catch (const std::exception& e) {
        LOGE(mLogPrefix + "Exception during parsing: " << e.what());
        removePeerInternal(socket.getFD(), Status::PARSING_ERROR);
        return true;
    }

    // LOGT(mLogPrefix + "Signal callback for methodID: " << methodID << "; messageID: " << messageID);
    try {
        signalCallbacks->signal(socket.getFD(), data);
    } catch (const std::exception& e) {
        LOGE(mLogPrefix + "Exception in method handler: " << e.what());
        removePeerInternal(socket.getFD(), Status::NAUGHTY_PEER);
        return true;
    }

    return false;
}

bool Processor::onRemoteCall(const Socket& socket,
                             const MethodID methodID,
                             const MessageID messageID,
                             std::shared_ptr<MethodHandlers> methodCallbacks)
{
    LOGS(mLogPrefix + "Processor onRemoteCall; methodID: " << methodID << " messageID: " << messageID);
    // LOGI(mLogPrefix + "Remote call; methodID: " << methodID << " messageID: " << messageID);

    std::shared_ptr<void> data;
    try {
        LOGT(mLogPrefix + "Parsing incoming data");
        data = methodCallbacks->parse(socket.getFD());
    } catch (const std::exception& e) {
        LOGE(mLogPrefix + "Exception during parsing: " << e.what());
        removePeerInternal(socket.getFD(), Status::PARSING_ERROR);
        return true;
    }

    LOGT(mLogPrefix + "Process callback for methodID: " << methodID << "; messageID: " << messageID);
    std::shared_ptr<void> returnData;
    try {
        returnData = methodCallbacks->method(socket.getFD(), data);
    } catch (const std::exception& e) {
        LOGE(mLogPrefix + "Exception in method handler: " << e.what());
        removePeerInternal(socket.getFD(), Status::NAUGHTY_PEER);
        return true;
    }

    LOGT(mLogPrefix + "Sending return data; methodID: " << methodID << "; messageID: " << messageID);
    try {
        // Send the call with the socket
        Socket::Guard guard = socket.getGuard();
        socket.write(&RETURN_METHOD_ID, sizeof(RETURN_METHOD_ID));
        socket.write(&messageID, sizeof(messageID));
        methodCallbacks->serialize(socket.getFD(), returnData);
    } catch (const std::exception& e) {
        LOGE(mLogPrefix + "Exception during serialization: " << e.what());
        removePeerInternal(socket.getFD(), Status::SERIALIZATION_ERROR);
        return true;
    }

    return false;
}

bool Processor::handleEvent()
{
    LOGS(mLogPrefix + "Processor handleEvent");

    Lock lock(mStateMutex);

    auto request = mRequestQueue.pop();
    LOGD(mLogPrefix + "Got: " << request.requestID);

    switch (request.requestID) {
    case Event::METHOD:      return onMethodRequest(*request.get<MethodRequest>());
    case Event::SIGNAL:      return onSignalRequest(*request.get<SignalRequest>());
    case Event::ADD_PEER:    return onAddPeerRequest(*request.get<AddPeerRequest>());
    case Event::REMOVE_PEER: return onRemovePeerRequest(*request.get<RemovePeerRequest>());
    case Event::FINISH:      return onFinishRequest(*request.get<FinishRequest>());
    }

    return false;
}

bool Processor::onMethodRequest(MethodRequest& request)
{
    LOGS(mLogPrefix + "Processor onMethodRequest");
    std::shared_ptr<Socket> socketPtr;

    try {
        // Get the peer's socket
        socketPtr = mSockets.at(request.peerFD);
    } catch (const std::out_of_range&) {
        LOGE(mLogPrefix + "Peer disconnected. No socket with a peerFD: " << request.peerFD);

        // Pass the error to the processing callback
        IGNORE_EXCEPTIONS(request.process(Status::PEER_DISCONNECTED, request.data));

        return false;
    }

    if (mReturnCallbacks.count(request.messageID) != 0) {
        LOGE(mLogPrefix + "There already was a return callback for messageID: " << request.messageID);
    }
    mReturnCallbacks[request.messageID] = std::move(ReturnCallbacks(request.peerFD,
                                                                    std::move(request.parse),
                                                                    std::move(request.process)));

    try {
        // Send the call with the socket
        Socket::Guard guard = socketPtr->getGuard();
        socketPtr->write(&request.methodID, sizeof(request.methodID));
        socketPtr->write(&request.messageID, sizeof(request.messageID));
        LOGT(mLogPrefix + "Serializing the message");
        request.serialize(socketPtr->getFD(), request.data);
    } catch (const std::exception& e) {
        LOGE(mLogPrefix + "Error during sending a method: " << e.what());

        // Inform about the error,
        IGNORE_EXCEPTIONS(mReturnCallbacks[request.messageID].process(Status::SERIALIZATION_ERROR, request.data));


        mReturnCallbacks.erase(request.messageID);
        removePeerInternal(request.peerFD, Status::SERIALIZATION_ERROR);

        return true;

    }

    return false;
}

bool Processor::onSignalRequest(SignalRequest& request)
{
    LOGS(mLogPrefix + "Processor onSignalRequest");

    std::shared_ptr<Socket> socketPtr;
    try {
        // Get the peer's socket
        socketPtr = mSockets.at(request.peerFD);
    } catch (const std::out_of_range&) {
        LOGE(mLogPrefix + "Peer disconnected. No socket with a peerFD: " << request.peerFD);
        return false;
    }

    try {
        // Send the call with the socket
        Socket::Guard guard = socketPtr->getGuard();
        socketPtr->write(&request.methodID, sizeof(request.methodID));
        socketPtr->write(&request.messageID, sizeof(request.messageID));
        request.serialize(socketPtr->getFD(), request.data);
    } catch (const std::exception& e) {
        LOGE(mLogPrefix + "Error during sending a signal: " << e.what());

        removePeerInternal(request.peerFD, Status::SERIALIZATION_ERROR);
        return true;
    }

    return false;
}

bool Processor::onAddPeerRequest(AddPeerRequest& request)
{
    LOGS(mLogPrefix + "Processor onAddPeerRequest");

    if (mSockets.size() > mMaxNumberOfPeers) {
        LOGE(mLogPrefix + "There are too many peers. I don't accept the connection with " << request.peerFD);
        return false;
    }
    if (mSockets.count(request.peerFD) != 0) {
        LOGE(mLogPrefix + "There already was a socket for peerFD: " << request.peerFD);
        return false;
    }

    mSockets[request.peerFD] = std::move(request.socketPtr);


    // Sending handled signals
    std::vector<MethodID> ids;
    for (const auto kv : mSignalsCallbacks) {
        ids.push_back(kv.first);
    }
    auto data = std::make_shared<RegisterSignalsMessage>(ids);
    callAsync<RegisterSignalsMessage, EmptyData>(REGISTER_SIGNAL_METHOD_ID,
                                                 request.peerFD,
                                                 data,
                                                 discardResultHandler<EmptyData>);


    resetPolling();

    if (mNewPeerCallback) {
        // Notify about the new user.
        LOGT(mLogPrefix + "Calling NewPeerCallback");
        mNewPeerCallback(request.peerFD);
    }

    LOGI(mLogPrefix + "New peer: " << request.peerFD);
    return true;
}

bool Processor::onRemovePeerRequest(RemovePeerRequest& request)
{
    LOGS(mLogPrefix + "Processor onRemovePeer");

    removePeerInternal(request.peerFD, Status::REMOVED_PEER);
    request.conditionPtr->notify_all();

    return true;
}

bool Processor::onFinishRequest(FinishRequest& request)
{
    LOGS(mLogPrefix + "Processor onFinishRequest");

    // Clean the mRequestQueue
    while (!mRequestQueue.isEmpty()) {
        auto request = mRequestQueue.pop();
        LOGE(mLogPrefix + "Got: " << request.requestID << " after FINISH");

        switch (request.requestID) {
        case Event::METHOD: {
            auto requestPtr = request.get<MethodRequest>();
            IGNORE_EXCEPTIONS(requestPtr->process(Status::CLOSING, requestPtr->data));
            break;
        }
        case Event::REMOVE_PEER: {
            onRemovePeerRequest(*request.get<RemovePeerRequest>());
            break;
        }
        case Event::SIGNAL:
        case Event::ADD_PEER:
        case Event::FINISH:
            break;
        }
    }

    mIsRunning = false;

    request.conditionPtr->notify_all();
    return true;
}

std::ostream& operator<<(std::ostream& os, const Processor::Event& event)
{
    switch (event) {

    case Processor::Event::FINISH: {
        os << "Event::FINISH";
        break;
    }

    case Processor::Event::METHOD: {
        os << "Event::METHOD";
        break;
    }

    case Processor::Event::SIGNAL: {
        os << "Event::SIGNAL";
        break;
    }

    case Processor::Event::ADD_PEER: {
        os << "Event::ADD_PEER";
        break;
    }

    case Processor::Event::REMOVE_PEER: {
        os << "Event::REMOVE_PEER";
        break;
    }
    }

    return os;
}

} // namespace ipc
} // namespace vasum
