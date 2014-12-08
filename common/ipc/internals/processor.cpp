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

#include "config.hpp"

#include "ipc/exception.hpp"
#include "ipc/internals/processor.hpp"
#include "ipc/internals/utils.hpp"

#include <cerrno>
#include <cstring>
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
        LOGE("Callback threw an error: " << e.what()); \
    }

const MethodID Processor::RETURN_METHOD_ID = std::numeric_limits<MethodID>::max();
const MethodID Processor::REGISTER_SIGNAL_METHOD_ID = std::numeric_limits<MethodID>::max() - 1;

Processor::Processor(const PeerCallback& newPeerCallback,
                     const PeerCallback& removedPeerCallback,
                     const unsigned int maxNumberOfPeers)
    : mNewPeerCallback(newPeerCallback),
      mRemovedPeerCallback(removedPeerCallback),
      mMaxNumberOfPeers(maxNumberOfPeers)
{
    LOGT("Creating Processor");
    using namespace std::placeholders;

    addMethodHandlerInternal<EmptyData, RegisterSignalsMessage>(REGISTER_SIGNAL_METHOD_ID,
                                                                std::bind(&Processor::onNewSignals, this, _1, _2));

}

Processor::~Processor()
{
    LOGT("Destroying Processor");
    try {
        stop();
    } catch (IPCException& e) {
        LOGE("Error in Processor's destructor: " << e.what());
    }
    LOGT("Destroyed Processor");
}

bool Processor::isStarted()
{
    return mThread.joinable();
}

void Processor::start()
{
    LOGT("Starting Processor");
    if (!isStarted()) {
        mThread = std::thread(&Processor::run, this);
    }
    LOGT("Started Processor");
}

void Processor::stop()
{
    LOGT("Stopping Processor");
    if (isStarted()) {
        mEventQueue.send(Event::FINISH);
        mThread.join();
    }
    LOGT("Stopped Processor");
}

void Processor::setNewPeerCallback(const PeerCallback& newPeerCallback)
{
    Lock lock(mCallbacksMutex);
    mNewPeerCallback = newPeerCallback;
}

void Processor::setRemovedPeerCallback(const PeerCallback& removedPeerCallback)
{
    Lock lock(mCallbacksMutex);
    mRemovedPeerCallback = removedPeerCallback;
}

FileDescriptor Processor::getEventFD()
{
    return mEventQueue.getFD();
}

void Processor::removeMethod(const MethodID methodID)
{
    LOGT("Removing method " << methodID);
    Lock lock(mCallsMutex);
    mMethodsCallbacks.erase(methodID);
}

FileDescriptor Processor::addPeer(const std::shared_ptr<Socket>& socketPtr)
{
    LOGT("Adding socket");
    FileDescriptor peerFD;
    {
        Lock lock(mSocketsMutex);
        peerFD = socketPtr->getFD();
        SocketInfo socketInfo(peerFD, std::move(socketPtr));
        mNewSockets.push(std::move(socketInfo));
        mEventQueue.send(Event::ADD_PEER);
    }
    LOGI("New peer added. Id: " << peerFD);

    return peerFD;
}

void Processor::removePeer(const FileDescriptor peerFD)
{
    std::shared_ptr<std::condition_variable> conditionPtr(new std::condition_variable());

    {
        Lock lock(mSocketsMutex);
        RemovePeerRequest request(peerFD, conditionPtr);
        mPeersToDelete.push(std::move(request));
        mEventQueue.send(Event::REMOVE_PEER);
    }


    auto isPeerDeleted = [&peerFD, this]()->bool {
        Lock lock(mSocketsMutex);
        return mSockets.count(peerFD) == 0;
    };

    std::mutex mutex;
    std::unique_lock<std::mutex> lock(mutex);
    conditionPtr->wait(lock, isPeerDeleted);
}

void Processor::removePeerInternal(const FileDescriptor peerFD, Status status)
{
    LOGW("Removing peer. ID: " << peerFD);
    {
        Lock lock(mSocketsMutex);
        mSockets.erase(peerFD);

        // Remove from signal addressees
        for (auto it = mSignalsPeers.begin(); it != mSignalsPeers.end();) {
            it->second.remove(peerFD);
            if (it->second.empty()) {
                it = mSignalsPeers.erase(it);
            } else {
                ++it;
            }
        }
    }

    {
        // Erase associated return value callbacks
        Lock lock(mReturnCallbacksMutex);

        std::shared_ptr<void> data;
        for (auto it = mReturnCallbacks.begin(); it != mReturnCallbacks.end();) {
            if (it->second.peerFD == peerFD) {
                IGNORE_EXCEPTIONS(it->second.process(status, data));
                it = mReturnCallbacks.erase(it);
            } else {
                ++it;
            }
        }
    }


    {
        Lock lock(mCallbacksMutex);
        if (mRemovedPeerCallback) {
            // Notify about the deletion
            mRemovedPeerCallback(peerFD);
        }
    }

    resetPolling();
}

void Processor::resetPolling()
{
    if (!isStarted()) {
        return;
    }

    LOGI("Resetting polling");
    // Setup polling on eventfd and sockets
    Lock lock(mSocketsMutex);
    mFDs.resize(mSockets.size() + 1);

    mFDs[0].fd = mEventQueue.getFD();
    mFDs[0].events = POLLIN;

    auto socketIt = mSockets.begin();
    for (unsigned int i = 1; i < mFDs.size(); ++i) {
        mFDs[i].fd = socketIt->second->getFD();
        mFDs[i].events = POLLIN | POLLHUP; // Listen for input events
        ++socketIt;
        // TODO: It's possible to block on writing to fd. Maybe listen for POLLOUT too?
    }
}

void Processor::run()
{
    resetPolling();

    mIsRunning = true;
    while (mIsRunning) {
        LOGT("Waiting for communication...");
        int ret = poll(mFDs.data(), mFDs.size(), -1 /*blocking call*/);
        LOGT("... incoming communication!");
        if (ret == -1 || ret == 0) {
            if (errno == EINTR) {
                continue;
            }
            LOGE("Error in poll: " << std::string(strerror(errno)));
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

    cleanCommunication();
}

bool Processor::handleLostConnections()
{
    std::vector<FileDescriptor> peersToRemove;
    {
        Lock lock(mSocketsMutex);
        for (unsigned int i = 1; i < mFDs.size(); ++i) {
            if (mFDs[i].revents & POLLHUP) {
                LOGI("Lost connection to peer: " << mFDs[i].fd);
                mFDs[i].revents &= ~(POLLHUP);
                peersToRemove.push_back(mFDs[i].fd);
            }
        }
    }

    for (const FileDescriptor peerFD : peersToRemove) {
        removePeerInternal(peerFD, Status::PEER_DISCONNECTED);
    }

    return !peersToRemove.empty();
}

bool Processor::handleLostConnection(const FileDescriptor peerFD)
{
    removePeerInternal(peerFD, Status::PEER_DISCONNECTED);
    return true;
}

bool Processor::handleInputs()
{
    std::vector<FileDescriptor> peersWithInput;
    {
        Lock lock(mSocketsMutex);
        for (unsigned int i = 1; i < mFDs.size(); ++i) {
            if (mFDs[i].revents & POLLIN) {
                mFDs[i].revents &= ~(POLLIN);
                peersWithInput.push_back(mFDs[i].fd);
            }
        }
    }

    bool pollChanged = false;
    // Handle input outside the critical section
    for (const FileDescriptor peerFD : peersWithInput) {
        pollChanged = pollChanged || handleInput(peerFD);
    }
    return pollChanged;
}

bool Processor::handleInput(const FileDescriptor peerFD)
{
    LOGT("Handle incoming data");

    std::shared_ptr<Socket> socketPtr;
    try {
        socketPtr = mSockets.at(peerFD);
    } catch (const std::out_of_range&) {
        LOGE("No such peer: " << peerFD);
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
            LOGE("Error during reading the socket");
            removePeerInternal(socketPtr->getFD(), Status::NAUGHTY_PEER);
            return true;
        }

        if (methodID == RETURN_METHOD_ID) {
            return onReturnValue(*socketPtr, messageID);

        } else {
            Lock lock(mCallsMutex);
            if (mMethodsCallbacks.count(methodID)) {
                // Method
                std::shared_ptr<MethodHandlers> methodCallbacks = mMethodsCallbacks.at(methodID);
                lock.unlock();
                return onRemoteCall(*socketPtr, methodID, messageID, methodCallbacks);

            } else if (mSignalsCallbacks.count(methodID)) {
                // Signal
                std::shared_ptr<SignalHandlers> signalCallbacks = mSignalsCallbacks.at(methodID);
                lock.unlock();
                return onRemoteSignal(*socketPtr, methodID, messageID, signalCallbacks);

            } else {
                // Nothing
                lock.unlock();
                LOGW("No method or signal callback for methodID: " << methodID);
                removePeerInternal(socketPtr->getFD(), Status::NAUGHTY_PEER);
                return true;
            }
        }
    }
}

std::shared_ptr<Processor::EmptyData> Processor::onNewSignals(const FileDescriptor peerFD,
                                                              std::shared_ptr<RegisterSignalsMessage>& data)
{
    LOGD("New signals for peer: " << peerFD);
    Lock lock(mSocketsMutex);
    for (MethodID methodID : data->ids) {
        mSignalsPeers[methodID].push_back(peerFD);
    }

    return std::make_shared<EmptyData>();
}

bool Processor::onReturnValue(const Socket& socket,
                              const MessageID messageID)
{
    LOGI("Return value for messageID: " << messageID);
    ReturnCallbacks returnCallbacks;
    try {
        Lock lock(mReturnCallbacksMutex);
        LOGT("Getting the return callback");
        returnCallbacks = std::move(mReturnCallbacks.at(messageID));
        mReturnCallbacks.erase(messageID);
    } catch (const std::out_of_range&) {
        LOGW("No return callback for messageID: " << messageID);
        removePeerInternal(socket.getFD(), Status::NAUGHTY_PEER);
        return true;
    }

    std::shared_ptr<void> data;
    try {
        LOGT("Parsing incoming return data");
        data = returnCallbacks.parse(socket.getFD());
    } catch (const std::exception& e) {
        LOGE("Exception during parsing: " << e.what());
        IGNORE_EXCEPTIONS(returnCallbacks.process(Status::PARSING_ERROR, data));
        removePeerInternal(socket.getFD(), Status::PARSING_ERROR);
        return true;
    }

    LOGT("Process return value callback for messageID: " << messageID);
    IGNORE_EXCEPTIONS(returnCallbacks.process(Status::OK, data));

    return false;
}

bool Processor::onRemoteSignal(const Socket& socket,
                               const MethodID methodID,
                               const MessageID messageID,
                               std::shared_ptr<SignalHandlers> signalCallbacks)
{
    LOGI("Remote signal; methodID: " << methodID << " messageID: " << messageID);

    std::shared_ptr<void> data;
    try {
        LOGT("Parsing incoming data");
        data = signalCallbacks->parse(socket.getFD());
    } catch (const std::exception& e) {
        LOGE("Exception during parsing: " << e.what());
        removePeerInternal(socket.getFD(), Status::PARSING_ERROR);
        return true;
    }

    LOGT("Signal callback for methodID: " << methodID << "; messageID: " << messageID);
    try {
        signalCallbacks->signal(socket.getFD(), data);
    } catch (const std::exception& e) {
        LOGE("Exception in method handler: " << e.what());
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
    LOGI("Remote call; methodID: " << methodID << " messageID: " << messageID);

    std::shared_ptr<void> data;
    try {
        LOGT("Parsing incoming data");
        data = methodCallbacks->parse(socket.getFD());
    } catch (const std::exception& e) {
        LOGE("Exception during parsing: " << e.what());
        removePeerInternal(socket.getFD(), Status::PARSING_ERROR);
        return true;
    }

    LOGT("Process callback for methodID: " << methodID << "; messageID: " << messageID);
    std::shared_ptr<void> returnData;
    try {
        returnData = methodCallbacks->method(socket.getFD(), data);
    } catch (const std::exception& e) {
        LOGE("Exception in method handler: " << e.what());
        removePeerInternal(socket.getFD(), Status::NAUGHTY_PEER);
        return true;
    }

    LOGT("Sending return data; methodID: " << methodID << "; messageID: " << messageID);
    try {
        // Send the call with the socket
        Socket::Guard guard = socket.getGuard();
        socket.write(&RETURN_METHOD_ID, sizeof(RETURN_METHOD_ID));
        socket.write(&messageID, sizeof(messageID));
        methodCallbacks->serialize(socket.getFD(), returnData);
    } catch (const std::exception& e) {
        LOGE("Exception during serialization: " << e.what());
        removePeerInternal(socket.getFD(), Status::SERIALIZATION_ERROR);
        return true;
    }

    return false;
}

bool Processor::handleEvent()
{
    switch (mEventQueue.receive()) {
    case Event::FINISH: {
        LOGD("Event FINISH");
        mIsRunning = false;
        return false;
    }

    case Event::CALL: {
        LOGD("Event CALL");
        return onCall();
    }

    case Event::ADD_PEER: {
        LOGD("Event ADD_PEER");
        return onNewPeer();
    }

    case Event::REMOVE_PEER: {
        LOGD("Event REMOVE_PEER");
        return onRemovePeer();
    }
    }

    return false;
}

bool Processor::onNewPeer()
{
    SocketInfo socketInfo;
    {
        Lock lock(mSocketsMutex);

        socketInfo = std::move(mNewSockets.front());
        mNewSockets.pop();

        if (mSockets.size() > mMaxNumberOfPeers) {
            LOGE("There are too many peers. I don't accept the connection with " << socketInfo.peerFD);
            return false;
        }
        if (mSockets.count(socketInfo.peerFD) != 0) {
            LOGE("There already was a socket for peerFD: " << socketInfo.peerFD);
            return false;
        }

        mSockets[socketInfo.peerFD] = std::move(socketInfo.socketPtr);
    }


    // Broadcast the new signal to peers
    LOGW("Sending handled signals");
    std::list<FileDescriptor> peersFDs;
    {
        Lock lock(mSocketsMutex);
        for (const auto kv : mSockets) {
            peersFDs.push_back(kv.first);
        }
    }

    std::vector<MethodID> ids;
    {
        Lock lock(mSocketsMutex);
        for (const auto kv : mSignalsCallbacks) {
            ids.push_back(kv.first);
        }
    }
    auto data = std::make_shared<RegisterSignalsMessage>(ids);

    for (const FileDescriptor peerFD : peersFDs) {
        callInternal<RegisterSignalsMessage, EmptyData>(REGISTER_SIGNAL_METHOD_ID,
                                                        peerFD,
                                                        data,
                                                        discardResultHandler<EmptyData>);
    }
    LOGW("Sent handled signals");


    resetPolling();

    {
        Lock lock(mCallbacksMutex);
        if (mNewPeerCallback) {
            // Notify about the new user.
            mNewPeerCallback(socketInfo.peerFD);
        }
    }
    return true;
}

bool Processor::onRemovePeer()
{
    RemovePeerRequest request;
    {
        Lock lock(mSocketsMutex);
        request = std::move(mPeersToDelete.front());
        mPeersToDelete.pop();
    }

    removePeerInternal(request.peerFD, Status::REMOVED_PEER);
    request.conditionPtr->notify_all();
    return true;
}

CallQueue::Call Processor::getCall()
{
    Lock lock(mCallsMutex);
    return mCalls.pop();
}

bool Processor::onCall()
{
    LOGT("Handle call (from another thread) to send a message.");
    CallQueue::Call call = getCall();

    std::shared_ptr<Socket> socketPtr;
    try {
        // Get the peer's socket
        Lock lock(mSocketsMutex);
        socketPtr = mSockets.at(call.peerFD);
    } catch (const std::out_of_range&) {
        LOGE("Peer disconnected. No socket with a peerFD: " << call.peerFD);
        IGNORE_EXCEPTIONS(call.process(Status::PEER_DISCONNECTED, call.data));
        return false;
    }

    if (call.parse && call.process) {
        // Set what to do with the return message, but only if needed
        Lock lock(mReturnCallbacksMutex);
        if (mReturnCallbacks.count(call.messageID) != 0) {
            LOGE("There already was a return callback for messageID: " << call.messageID);
        }
        mReturnCallbacks[call.messageID] = std::move(ReturnCallbacks(call.peerFD,
                                                                     std::move(call.parse),
                                                                     std::move(call.process)));
    }

    try {
        // Send the call with the socket
        Socket::Guard guard = socketPtr->getGuard();
        socketPtr->write(&call.methodID, sizeof(call.methodID));
        socketPtr->write(&call.messageID, sizeof(call.messageID));
        call.serialize(socketPtr->getFD(), call.data);
    } catch (const std::exception& e) {
        LOGE("Error during sending a message: " << e.what());

        // Inform about the error
        IGNORE_EXCEPTIONS(mReturnCallbacks[call.messageID].process(Status::SERIALIZATION_ERROR, call.data));

        {
            Lock lock(mReturnCallbacksMutex);
            mReturnCallbacks.erase(call.messageID);
        }

        removePeerInternal(call.peerFD, Status::SERIALIZATION_ERROR);
        return true;
    }

    return false;
}

void Processor::cleanCommunication()
{
    while (!mEventQueue.isEmpty()) {
        switch (mEventQueue.receive()) {
        case Event::FINISH: {
            LOGD("Event FINISH after FINISH");
            break;
        }
        case Event::CALL: {
            LOGD("Event CALL after FINISH");
            CallQueue::Call call = getCall();
            if (call.process) {
                IGNORE_EXCEPTIONS(call.process(Status::CLOSING, call.data));
            }
            break;
        }

        case Event::ADD_PEER: {
            LOGD("Event ADD_PEER after FINISH");
            break;
        }

        case Event::REMOVE_PEER: {
            LOGD("Event REMOVE_PEER after FINISH");
            RemovePeerRequest request;
            {
                Lock lock(mSocketsMutex);
                request = std::move(mPeersToDelete.front());
                mPeersToDelete.pop();
            }
            request.conditionPtr->notify_all();
            break;
        }
        }
    }
}

} // namespace ipc
} // namespace vasum
