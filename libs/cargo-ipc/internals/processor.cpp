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

#include "cargo-ipc/exception.hpp"
#include "cargo-ipc/internals/processor.hpp"
#include "cargo-fd/cargo-fd.hpp"
#include "cargo/exception.hpp"

#include <cerrno>
#include <cstring>
#include <csignal>
#include <stdexcept>
#include <cassert>

#include <sys/socket.h>
#include <limits>

using namespace utils;

namespace cargo {
namespace ipc {
namespace internals {

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
const MethodID Processor::ERROR_METHOD_ID = std::numeric_limits<MethodID>::max() - 2;

Processor::Processor(epoll::EventPoll& eventPoll,
                     const std::string& logName,
                     const PeerCallback& newPeerCallback,
                     const PeerCallback& removedPeerCallback,
                     const unsigned int maxNumberOfPeers)
    : mEventPoll(eventPoll),
      mLogPrefix(logName),
      mIsRunning(false),
      mNewPeerCallback(newPeerCallback),
      mRemovedPeerCallback(removedPeerCallback),
      mMaxNumberOfPeers(maxNumberOfPeers)
{
    LOGS(mLogPrefix + "Processor Constructor");

    using namespace std::placeholders;
    setSignalHandlerInternal<RegisterSignalsProtocolMessage>(REGISTER_SIGNAL_METHOD_ID,
                                                             std::bind(&Processor::onNewSignals, this, _1, _2));

    setSignalHandlerInternal<ErrorProtocolMessage>(ERROR_METHOD_ID, std::bind(&Processor::onErrorSignal, this, _1, _2));
}

Processor::~Processor()
{
    LOGS(mLogPrefix + "Processor Destructor");
    try {
        stop(false);
    } catch (std::exception& e) {
        LOGE(mLogPrefix + "Error in Processor's destructor: " << e.what());
    }
}

Processor::Peers::iterator Processor::getPeerInfoIterator(const FileDescriptor fd)
{
    return std::find_if(mPeerInfo.begin(), mPeerInfo.end(), [fd](const PeerInfo & peerInfo) {
        return fd == peerInfo.socketPtr->getFD();
    });
}

Processor::Peers::iterator Processor::getPeerInfoIterator(const PeerID & peerID)
{
    return std::find_if(mPeerInfo.begin(), mPeerInfo.end(), [peerID](const PeerInfo & peerInfo) {
        return peerID == peerInfo.peerID;
    });
}

bool Processor::isStarted()
{
    Lock lock(mStateMutex);
    return mIsRunning;
}

void Processor::start()
{
    LOGS(mLogPrefix + "Processor start");

    Lock lock(mStateMutex);
    if (!mIsRunning) {
        LOGI(mLogPrefix + "Processor start");
        mIsRunning = true;

        mEventPoll.addFD(mRequestQueue.getFD(), EPOLLIN, std::bind(&Processor::handleEvent, this));
    }
}

void Processor::stop(bool wait)
{
    LOGS(mLogPrefix + "Processor stop");

    if (isStarted()) {
        auto conditionPtr = std::make_shared<std::condition_variable>();
        {
            Lock lock(mStateMutex);
            auto request = std::make_shared<FinishRequest>(conditionPtr);
            mRequestQueue.pushBack(Event::FINISH, request);
        }

        if (wait) {
            LOGD(mLogPrefix + "Waiting for the Processor to stop");

            // Wait till the FINISH request is served
            Lock lock(mStateMutex);
            conditionPtr->wait(lock, [this]() {
                return !mIsRunning;
            });
            assert(mPeerInfo.empty());
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

void Processor::sendResult(const MethodID methodID,
                           const PeerID& peerID,
                           const MessageID& messageID,
                           const std::shared_ptr<void>& data)
{
    auto requestPtr = std::make_shared<SendResultRequest>(methodID, peerID, messageID, data);
    mRequestQueue.pushFront(Event::SEND_RESULT, requestPtr);
}

void Processor::sendError(const PeerID& peerID,
                          const MessageID& messageID,
                          const int errorCode,
                          const std::string& message)
{
    auto data = std::make_shared<ErrorProtocolMessage>(messageID, errorCode, message);
    signalInternal<ErrorProtocolMessage>(ERROR_METHOD_ID, peerID , data);
}

void Processor::sendVoid(const MethodID methodID,
                         const PeerID& peerID,
                         const MessageID& messageID)
{
    auto data = std::make_shared<EmptyData>();
    auto requestPtr = std::make_shared<SendResultRequest>(methodID, peerID, messageID, data);
    mRequestQueue.pushFront(Event::SEND_RESULT, requestPtr);
}

void Processor::removeMethod(const MethodID methodID)
{
    Lock lock(mStateMutex);
    mMethodsCallbacks.erase(methodID);
}

bool Processor::isHandled(const MethodID methodID)
{
    Lock lock(mStateMutex);
    return mMethodsCallbacks.count(methodID) > 0 ||
           mSignalsCallbacks.count(methodID) > 0 ;
}

PeerID Processor::addPeer(const std::shared_ptr<Socket>& socketPtr)
{
    LOGS(mLogPrefix + "Processor addPeer");
    Lock lock(mStateMutex);

    auto requestPtr = std::make_shared<AddPeerRequest>(socketPtr);
    mRequestQueue.pushBack(Event::ADD_PEER, requestPtr);

    LOGI(mLogPrefix + "Add Peer Request. Id: " << shortenPeerID(requestPtr->peerID)
                                   << ", fd: " << socketPtr->getFD());

    return requestPtr->peerID;
}

void Processor::removePeerSyncInternal(const PeerID& peerID, Lock& lock)
{
    LOGS(mLogPrefix + "Processor removePeer peerID: " << shortenPeerID(peerID));

    auto isPeerDeleted = [&peerID, this]()->bool {
        return getPeerInfoIterator(peerID) == mPeerInfo.end();
    };

    mRequestQueue.removeIf([peerID](Request & request) {
        return request.requestID == Event::ADD_PEER &&
               request.get<AddPeerRequest>()->peerID == peerID;
    });

    // Remove peer and wait till he's gone
    std::shared_ptr<std::condition_variable> conditionPtr(new std::condition_variable());

    auto request = std::make_shared<RemovePeerRequest>(peerID, conditionPtr);
    mRequestQueue.pushBack(Event::REMOVE_PEER, request);

    conditionPtr->wait(lock, isPeerDeleted);
}

void Processor::removePeerInternal(Peers::iterator peerIt, const std::exception_ptr& exceptionPtr)
{
    if (peerIt == mPeerInfo.end()) {
        LOGW("Peer already removed");
        return;
    }

    LOGS(mLogPrefix + "Processor removePeerInternal peerID: " << shortenPeerID(peerIt->peerID));
    LOGI(mLogPrefix + "Removing peer. peerID: " << shortenPeerID(peerIt->peerID));

    // Remove from signal addressees
    for (auto it = mSignalsPeers.begin(); it != mSignalsPeers.end();) {
        it->second.remove(peerIt->peerID);
        if (it->second.empty()) {
            it = mSignalsPeers.erase(it);
        } else {
            ++it;
        }
    }

    // Erase associated return value callbacks
    for (auto it = mReturnCallbacks.begin(); it != mReturnCallbacks.end();) {
        if (it->second.peerID == peerIt->peerID) {
            ResultBuilder resultBuilder(exceptionPtr);
            IGNORE_EXCEPTIONS(it->second.process(resultBuilder));
            it = mReturnCallbacks.erase(it);
        } else {
            ++it;
        }
    }

    if (mRemovedPeerCallback) {
        // Notify about the deletion
        mRemovedPeerCallback(peerIt->peerID, peerIt->socketPtr->getFD());
    }

    mPeerInfo.erase(peerIt);
}

void Processor::handleLostConnection(const FileDescriptor fd)
{
    Lock lock(mStateMutex);
    auto peerIt = getPeerInfoIterator(fd);
    removePeerInternal(peerIt,
                       std::make_exception_ptr(IPCPeerDisconnectedException()));
}

void Processor::handleInput(const FileDescriptor fd)
{
    LOGS(mLogPrefix + "Processor handleInput fd: " << fd);

    Lock lock(mStateMutex);

    auto peerIt = getPeerInfoIterator(fd);

    if (peerIt == mPeerInfo.end()) {
        LOGE(mLogPrefix + "No peer for fd: " << fd);
        return;
    }

    MessageHeader hdr;
    {
        try {
            // Read information about the incoming data
            Socket& socket = *peerIt->socketPtr;
            cargo::loadFromFD<MessageHeader>(socket.getFD(), hdr);
        } catch (const cargo::CargoException& e) {
            LOGE(mLogPrefix + "Error during reading the socket");
            removePeerInternal(peerIt,
                               std::make_exception_ptr(IPCNaughtyPeerException()));
            return;
        }

        if (hdr.methodID == RETURN_METHOD_ID) {
            onReturnValue(peerIt, hdr.messageID);
            return;
        } else {
            if (mMethodsCallbacks.count(hdr.methodID)) {
                // Method
                std::shared_ptr<MethodHandlers> methodCallbacks = mMethodsCallbacks.at(hdr.methodID);
                onRemoteMethod(peerIt, hdr.methodID, hdr.messageID, methodCallbacks);
            } else if (mSignalsCallbacks.count(hdr.methodID)) {
                // Signal
                std::shared_ptr<SignalHandlers> signalCallbacks = mSignalsCallbacks.at(hdr.methodID);
                onRemoteSignal(peerIt, hdr.methodID, hdr.messageID, signalCallbacks);

            } else {
                LOGW(mLogPrefix + "No method or signal callback for methodID: " << hdr.methodID);
                removePeerInternal(peerIt,
                                   std::make_exception_ptr(IPCNaughtyPeerException()));
            }
        }
    }
}

ipc::HandlerExitCode Processor::onNewSignals(const PeerID& peerID, std::shared_ptr<RegisterSignalsProtocolMessage>& data)
{
    LOGS(mLogPrefix + "Processor onNewSignals peerID: " << shortenPeerID(peerID));

    for (const MethodID methodID : data->ids) {
        mSignalsPeers[methodID].push_back(peerID);
    }

    return ipc::HandlerExitCode::SUCCESS;
}

ipc::HandlerExitCode Processor::onErrorSignal(const PeerID&, std::shared_ptr<ErrorProtocolMessage>& data)
{
    LOGS(mLogPrefix + "Processor onErrorSignal messageID: " << shortenMessageID(data->messageID));

    // If there is no return callback an out_of_range error will be thrown and peer will be removed
    ReturnCallbacks returnCallbacks = std::move(mReturnCallbacks.at(data->messageID));
    mReturnCallbacks.erase(data->messageID);

    ResultBuilder resultBuilder(std::make_exception_ptr(IPCUserException(data->code, data->message)));
    IGNORE_EXCEPTIONS(returnCallbacks.process(resultBuilder));

    return ipc::HandlerExitCode::SUCCESS;
}

void Processor::onReturnValue(Peers::iterator& peerIt,
                              const MessageID& messageID)
{
    LOGS(mLogPrefix + "Processor onReturnValue messageID: " << shortenMessageID(messageID));

    ReturnCallbacks returnCallbacks;
    try {
        LOGT(mLogPrefix + "Getting the return callback");
        returnCallbacks = std::move(mReturnCallbacks.at(messageID));
        mReturnCallbacks.erase(messageID);
    } catch (const std::out_of_range&) {
        LOGW(mLogPrefix + "No return callback for messageID: " << shortenMessageID(messageID));
        removePeerInternal(peerIt,
                           std::make_exception_ptr(IPCNaughtyPeerException()));
        return;
    }

    std::shared_ptr<void> data;
    try {
        LOGT(mLogPrefix + "Parsing incoming return data");
        data = returnCallbacks.parse(peerIt->socketPtr->getFD());
    } catch (const std::exception& e) {
        LOGE(mLogPrefix + "Exception during parsing: " << e.what());
        ResultBuilder resultBuilder(std::make_exception_ptr(IPCParsingException()));
        IGNORE_EXCEPTIONS(returnCallbacks.process(resultBuilder));
        removePeerInternal(peerIt,
                           std::make_exception_ptr(IPCParsingException()));
        return;
    }

    ResultBuilder resultBuilder(data);
    IGNORE_EXCEPTIONS(returnCallbacks.process(resultBuilder));
}

void Processor::onRemoteSignal(Peers::iterator& peerIt,
                               __attribute__((unused)) const MethodID methodID,
                               __attribute__((unused)) const MessageID& messageID,
                               std::shared_ptr<SignalHandlers> signalCallbacks)
{
    LOGS(mLogPrefix + "Processor onRemoteSignal; methodID: " << methodID
                                           << " messageID: " << shortenMessageID(messageID));

    std::shared_ptr<void> data;
    try {
        LOGT(mLogPrefix + "Parsing incoming data");
        data = signalCallbacks->parse(peerIt->socketPtr->getFD());
    } catch (const std::exception& e) {
        LOGE(mLogPrefix + "Exception during parsing: " << e.what());
        removePeerInternal(peerIt,
                           std::make_exception_ptr(IPCParsingException()));
        return;
    }

    try {
        auto leaveHandler = signalCallbacks->signal(peerIt->peerID, data);

        if(leaveHandler == HandlerExitCode::REMOVE_HANDLER) {
            LOGI("Signal handler requested deletion (returned REMOVE_HANDLER): " << methodID);
            mSignalsCallbacks.erase(methodID);
        }
    } catch (const IPCUserException& e) {
        LOGW("Discarded user's exception");
        return;
    } catch (const std::exception& e) {
        LOGE(mLogPrefix + "Exception in method handler: " << e.what());
        removePeerInternal(peerIt,
                           std::make_exception_ptr(IPCNaughtyPeerException()));
        return;
    }
}

void Processor::onRemoteMethod(Peers::iterator& peerIt,
                               const MethodID methodID,
                               const MessageID& messageID,
                               std::shared_ptr<MethodHandlers> methodCallbacks)
{
    LOGS(mLogPrefix + "Processor onRemoteMethod; methodID: " << methodID
                                           << " messageID: " << shortenMessageID(messageID));

    std::shared_ptr<void> data;
    try {
        LOGT(mLogPrefix + "Parsing incoming data");
        data = methodCallbacks->parse(peerIt->socketPtr->getFD());
    } catch (const std::exception& e) {
        LOGE(mLogPrefix + "Exception during parsing: " << e.what());
        removePeerInternal(peerIt,
                           std::make_exception_ptr(IPCParsingException()));
        return;
    }

    LOGT(mLogPrefix + "Process callback for methodID: " << methodID
                                     << "; messageID: " << shortenMessageID(messageID));
    try {
        auto methodResultPtr = std::make_shared<MethodResult>(*this, methodID, messageID, peerIt->peerID);
        auto leaveHandler = methodCallbacks->method(peerIt->peerID, data, methodResultPtr);

        if(leaveHandler == HandlerExitCode::SUCCESS) {
            // Leave the handler
            return;
        }

        LOGI("Method handler requested deletion (returned REMOVE_HANDLER): " << methodID);
        if(!methodResultPtr.unique()) {
            LOGE("There is a MethodResult object referencing this method, can't delete: " << methodID);
        }

        // We can't just remove the callback here,
        // MethodResult called sendResult/sendError/sendVoid and the request can still be in the queue.
        // TODO: There can be another request for this method
        auto requestPtr = std::make_shared<RemoveMethodRequest>(methodID);
        mRequestQueue.pushBack(Event::REMOVE_METHOD, requestPtr);
    } catch (const IPCUserException& e) {
        LOGW("User's exception");
        sendError(peerIt->peerID, messageID, e.getCode(), e.what());
        return;
    } catch (const std::exception& e) {
        LOGE(mLogPrefix + "Exception in method handler: " << e.what());
        removePeerInternal(peerIt,
                           std::make_exception_ptr(IPCNaughtyPeerException()));
        return;
    }
}

void Processor::handleEvent()
{
    LOGS(mLogPrefix + "Processor handleEvent");

    Lock lock(mStateMutex);

    auto request = mRequestQueue.pop();
    LOGD(mLogPrefix + "Got: " << request.requestID);

    switch (request.requestID) {
    case Event::METHOD:
        onMethodRequest(*request.get<MethodRequest>());
        break;
    case Event::SIGNAL:
        onSignalRequest(*request.get<SignalRequest>());
        break;
    case Event::ADD_PEER:
        onAddPeerRequest(*request.get<AddPeerRequest>());
        break;
    case Event::REMOVE_PEER:
        onRemovePeerRequest(*request.get<RemovePeerRequest>());
        break;
    case Event::SEND_RESULT:
        onSendResultRequest(*request.get<SendResultRequest>());
        break;
    case Event::REMOVE_METHOD:
        onRemoveMethodRequest(*request.get<RemoveMethodRequest>());
        break;
    case Event::FINISH:
        onFinishRequest(*request.get<FinishRequest>());
        break;
    }
}

void Processor::onMethodRequest(MethodRequest& request)
{
    LOGS(mLogPrefix + "Processor onMethodRequest");

    auto peerIt = getPeerInfoIterator(request.peerID);

    if (peerIt == mPeerInfo.end()) {
        LOGE(mLogPrefix + "Peer disconnected. No user with a peerID: "
             << shortenPeerID(request.peerID));

        // Pass the error to the processing callback
        ResultBuilder resultBuilder(std::make_exception_ptr(IPCPeerDisconnectedException()));
        IGNORE_EXCEPTIONS(request.process(resultBuilder));

        return;
    }

    if (mReturnCallbacks.count(request.messageID) != 0) {
        LOGE(mLogPrefix + "There already was a return callback for messageID: "
             << shortenMessageID(request.messageID));
    }
    mReturnCallbacks[request.messageID] = ReturnCallbacks(peerIt->peerID,
                                                          std::move(request.parse),
                                                          std::move(request.process));

    MessageHeader hdr;
    try {
        // Send the call with the socket
        Socket& socket = *peerIt->socketPtr;
        hdr.methodID = request.methodID;
        hdr.messageID = request.messageID;
        cargo::saveToFD<MessageHeader>(socket.getFD(), hdr);
        LOGT(mLogPrefix + "Serializing the message");
        request.serialize(socket.getFD(), request.data);
    } catch (const std::exception& e) {
        LOGE(mLogPrefix + "Error during sending a method: " << e.what());

        // Inform about the error
        ResultBuilder resultBuilder(std::make_exception_ptr(IPCSerializationException()));
        IGNORE_EXCEPTIONS(mReturnCallbacks[request.messageID].process(resultBuilder));


        mReturnCallbacks.erase(request.messageID);
        removePeerInternal(peerIt,
                           std::make_exception_ptr(IPCSerializationException()));
    }
}

void Processor::onSignalRequest(SignalRequest& request)
{
    LOGS(mLogPrefix + "Processor onSignalRequest");

    auto peerIt = getPeerInfoIterator(request.peerID);

    if (peerIt == mPeerInfo.end()) {
        LOGE(mLogPrefix + "Peer disconnected. No user for peerID: "
             << shortenPeerID(request.peerID));
        return;
    }

    MessageHeader hdr;
    try {
        // Send the call with the socket
        Socket& socket = *peerIt->socketPtr;
        hdr.methodID = request.methodID;
        hdr.messageID = request.messageID;
        cargo::saveToFD<MessageHeader>(socket.getFD(), hdr);
        request.serialize(socket.getFD(), request.data);
    } catch (const std::exception& e) {
        LOGE(mLogPrefix + "Error during sending a signal: " << e.what());

        removePeerInternal(peerIt,
                           std::make_exception_ptr(IPCSerializationException()));
    }
}

void Processor::onAddPeerRequest(AddPeerRequest& request)
{
    LOGS(mLogPrefix + "Processor onAddPeerRequest");

    if (mPeerInfo.size() > mMaxNumberOfPeers) {
        LOGE(mLogPrefix + "There are too many peers. I don't accept the connection with "
             << shortenPeerID(request.peerID));
        return;
    }

    if (getPeerInfoIterator(request.peerID) != mPeerInfo.end()) {
        LOGE(mLogPrefix + "There already was a socket for peerID: "
             << shortenPeerID(request.peerID));
        return;
    }

    PeerInfo peerInfo(request.peerID, request.socketPtr);
    mPeerInfo.push_back(std::move(peerInfo));


    // Sending handled signals
    std::vector<MethodID> ids;
    for (const auto kv : mSignalsCallbacks) {
        ids.push_back(kv.first);
    }
    auto data = std::make_shared<RegisterSignalsProtocolMessage>(ids);
    signalInternal<RegisterSignalsProtocolMessage>(REGISTER_SIGNAL_METHOD_ID,
                                                   request.peerID,
                                                   data);

    if (mNewPeerCallback) {
        // Notify about the new user.
        LOGT(mLogPrefix + "Calling NewPeerCallback");
        mNewPeerCallback(request.peerID, request.socketPtr->getFD());
    }

    LOGI(mLogPrefix + "New peerID: " << shortenPeerID(request.peerID));
}

void Processor::onRemovePeerRequest(RemovePeerRequest& request)
{
    LOGS(mLogPrefix + "Processor onRemovePeer");

    removePeerInternal(getPeerInfoIterator(request.peerID),
                       std::make_exception_ptr(IPCRemovedPeerException()));

    request.conditionPtr->notify_all();
}

void Processor::onSendResultRequest(SendResultRequest& request)
{
    LOGS(mLogPrefix + "Processor onMethodRequest");

    auto peerIt = getPeerInfoIterator(request.peerID);

    if (peerIt == mPeerInfo.end()) {
        LOGE(mLogPrefix + "Peer disconnected, no result is sent. No user with a peerID: "
             << shortenPeerID(request.peerID));
        return;
    }

    std::shared_ptr<MethodHandlers> methodCallbacks;
    try {
        methodCallbacks = mMethodsCallbacks.at(request.methodID);
    } catch (const std::out_of_range&) {
        LOGW(mLogPrefix + "No method, might have been deleted. methodID: " << request.methodID);
        return;
    }

    MessageHeader hdr;
    try {
        // Send the call with the socket
        Socket& socket = *peerIt->socketPtr;
        hdr.methodID = RETURN_METHOD_ID;
        hdr.messageID = request.messageID;
        cargo::saveToFD<MessageHeader>(socket.getFD(), hdr);
        LOGT(mLogPrefix + "Serializing the message");
        methodCallbacks->serialize(socket.getFD(), request.data);
    } catch (const std::exception& e) {
        LOGE(mLogPrefix + "Error during sending a method: " << e.what());

        // Inform about the error
        ResultBuilder resultBuilder(std::make_exception_ptr(IPCSerializationException()));
        IGNORE_EXCEPTIONS(mReturnCallbacks[request.messageID].process(resultBuilder));


        mReturnCallbacks.erase(request.messageID);
        removePeerInternal(peerIt,
                           std::make_exception_ptr(IPCSerializationException()));
    }
}

void Processor::onRemoveMethodRequest(RemoveMethodRequest& request)
{
    mMethodsCallbacks.erase(request.methodID);
}

void Processor::onFinishRequest(FinishRequest& requestFinisher)
{
    LOGS(mLogPrefix + "Processor onFinishRequest");

    // Clean the mRequestQueue
    std::vector<std::shared_ptr<FinishRequest>> remainingFinishRequests;
    while (!mRequestQueue.isEmpty()) {
        auto request = mRequestQueue.pop();
        LOGE(mLogPrefix + "Got: " << request.requestID << " after FINISH");

        switch (request.requestID) {
        case Event::METHOD: {
            auto requestPtr = request.get<MethodRequest>();
            ResultBuilder resultBuilder(std::make_exception_ptr(IPCClosingException()));
            IGNORE_EXCEPTIONS(requestPtr->process(resultBuilder));
            break;
        }
        case Event::REMOVE_PEER: {
            onRemovePeerRequest(*request.get<RemovePeerRequest>());
            break;
        }
        case Event::SEND_RESULT: {
            onSendResultRequest(*request.get<SendResultRequest>());
            break;
        }
        case Event::FINISH:
            remainingFinishRequests.push_back(request.get<FinishRequest>());
            break;
        case Event::SIGNAL:
        case Event::ADD_PEER:
        case Event::REMOVE_METHOD:
            break;
        }
    }

    // Close peers
    while (!mPeerInfo.empty()) {
        removePeerInternal(--mPeerInfo.end(),
                           std::make_exception_ptr(IPCClosingException()));
    }

    mEventPoll.removeFD(mRequestQueue.getFD());
    mIsRunning = false;
    requestFinisher.conditionPtr->notify_all();
    for(auto & c : remainingFinishRequests) {
        c->conditionPtr->notify_all();
    }
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

    case Processor::Event::REMOVE_METHOD: {
        os << "Event::REMOVE_METHOD";
        break;
    }

    case Processor::Event::SEND_RESULT: {
        os << "Event::SEND_RESULT";
        break;
    }
    }

    return os;
}

} // namespace internals
} // namespace ipc
} // namespace cargo
