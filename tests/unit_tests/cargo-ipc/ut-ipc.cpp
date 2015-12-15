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
 * @brief   Tests of the IPC
 */

#include "config.hpp"

#include "ut.hpp"

#include "cargo-ipc/service.hpp"
#include "cargo-ipc/client.hpp"
#include "cargo-ipc/types.hpp"
#include "cargo-ipc/unique-id.hpp"
#include "cargo-ipc/result.hpp"
#include "cargo-ipc/epoll/thread-dispatcher.hpp"
#include "cargo-ipc/epoll/glib-dispatcher.hpp"
#include "utils/channel.hpp"
#include "utils/glib-loop.hpp"
#include "utils/latch.hpp"
#include "utils/value-latch.hpp"
#include "utils/scoped-dir.hpp"

#include "cargo/fields.hpp"
#include "logger/logger.hpp"

#include <boost/filesystem.hpp>
#include <fstream>
#include <atomic>
#include <string>
#include <thread>
#include <chrono>
#include <utility>
#include <future>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>


using namespace cargo::ipc;
using namespace epoll;
using namespace utils;
using namespace std::placeholders;
namespace fs = boost::filesystem;

// Timeout for sending one message
const int TIMEOUT = 1000 /*ms*/;

// Time that won't cause "TIMEOUT" methods to throw
const int SHORT_OPERATION_TIME = TIMEOUT / 100;

// Time that will cause "TIMEOUT" methods to throw
const int LONG_OPERATION_TIME = 1000 + TIMEOUT;

const std::string TEST_DIR = "/tmp/ut-ipc";
const std::string SOCKET_PATH = TEST_DIR + "/test.socket";
const std::string TEST_FILE = TEST_DIR + "/file.txt";

struct FixtureBase {
    ScopedDir mTestPathGuard;

    FixtureBase()
        : mTestPathGuard(TEST_DIR)
    {
    }
};

struct ThreadedFixture : FixtureBase {
    ThreadDispatcher dispatcher;

    EventPoll& getPoll() {
        return dispatcher.getPoll();
    }
};

struct GlibFixture : FixtureBase {
    ScopedGlibLoop glibLoop;
    GlibDispatcher dispatcher;

    EventPoll& getPoll() {
        return dispatcher.getPoll();
    }
};

struct SendData {
    int intVal;
    SendData(int i): intVal(i) {}

    CARGO_REGISTER
    (
        intVal
    )
};

struct RecvData {
    int intVal;
    RecvData(): intVal(-1) {}

    CARGO_REGISTER
    (
        intVal
    )
};

struct FDData {
    cargo::FileDescriptor fd;
    FDData(int fd = -1): fd(fd) {}

    CARGO_REGISTER
    (
        fd
    )
};

struct LongSendData {
    LongSendData(int i, int waitTime): mSendData(i), mWaitTime(waitTime), intVal(i) {}

    template<typename Visitor>
    void accept(Visitor visitor)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(mWaitTime));
        mSendData.accept(visitor);
    }
    template<typename Visitor>
    void accept(Visitor visitor) const
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(mWaitTime));
        mSendData.accept(visitor);
    }

    SendData mSendData;
    int mWaitTime;
    int intVal;
};

struct EmptyData {
    CARGO_REGISTER_EMPTY
};

struct ThrowOnAcceptData {
    template<typename Visitor>
    static void accept(Visitor)
    {
        throw std::runtime_error("intentional failure in accept");
    }
};

HandlerExitCode returnEmptyCallback(const PeerID,
                                                std::shared_ptr<EmptyData>&,
                                                MethodResult::Pointer methodResult)
{
    methodResult->setVoid();
    return HandlerExitCode::SUCCESS;
}

HandlerExitCode returnDataCallback(const PeerID,
                                   std::shared_ptr<RecvData>&,
                                   MethodResult::Pointer methodResult)
{
    auto returnData = std::make_shared<SendData>(1);
    methodResult->set(returnData);
    return HandlerExitCode::SUCCESS;
}

HandlerExitCode echoCallback(const PeerID,
                             std::shared_ptr<RecvData>& data,
                             MethodResult::Pointer methodResult)
{
    auto returnData = std::make_shared<SendData>(data->intVal);
    methodResult->set(returnData);
    return HandlerExitCode::SUCCESS;
}

HandlerExitCode longEchoCallback(const PeerID,
                                 std::shared_ptr<RecvData>& data,
                                 MethodResult::Pointer methodResult)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(LONG_OPERATION_TIME));
    auto returnData = std::make_shared<SendData>(data->intVal);
    methodResult->set(returnData);
    return HandlerExitCode::SUCCESS;
}

HandlerExitCode shortEchoCallback(const PeerID,
                                  std::shared_ptr<RecvData>& data,
                                  MethodResult::Pointer methodResult)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_OPERATION_TIME));
    auto returnData = std::make_shared<SendData>(data->intVal);
    methodResult->set(returnData);
    return HandlerExitCode::SUCCESS;
}

PeerID connectPeer(Service& s, Client& c)
{
    // Connects the Client to the Service and returns Clients PeerID
    ValueLatch<PeerID> peerIDLatch;
    auto newPeerCallback = [&peerIDLatch](const PeerID newID, const FileDescriptor) {
        peerIDLatch.set(newID);
    };

    s.setNewPeerCallback(newPeerCallback);

    if (!s.isStarted()) {
        s.start();
    }

    c.start();

    PeerID peerID = peerIDLatch.get(TIMEOUT);
    s.setNewPeerCallback(nullptr);
    BOOST_REQUIRE_NE(peerID, static_cast<std::string>(cargo::ipc::UniqueID()));
    return peerID;
}

void testEcho(Client& c, const MethodID methodID)
{
    std::shared_ptr<SendData> sentData(new SendData(34));
    std::shared_ptr<RecvData> recvData = c.callSync<SendData, RecvData>(methodID, sentData, TIMEOUT);
    BOOST_REQUIRE(recvData);
    BOOST_CHECK_EQUAL(recvData->intVal, sentData->intVal);
}

void testEcho(Service& s, const MethodID methodID, const PeerID& peerID)
{
    std::shared_ptr<SendData> sentData(new SendData(56));
    std::shared_ptr<RecvData> recvData = s.callSync<SendData, RecvData>(methodID, peerID, sentData, TIMEOUT);
    BOOST_REQUIRE(recvData);
    BOOST_CHECK_EQUAL(recvData->intVal, sentData->intVal);
}

BOOST_AUTO_TEST_SUITE(IPCSuite)

MULTI_FIXTURE_TEST_CASE(ConstructorDestructor, F, ThreadedFixture, GlibFixture)
{
    Service s(F::getPoll(), SOCKET_PATH);
    Client c(F::getPoll(), SOCKET_PATH);
}

MULTI_FIXTURE_TEST_CASE(ServiceAddRemoveMethod, F, ThreadedFixture, GlibFixture)
{
    Service s(F::getPoll(), SOCKET_PATH);
    s.setMethodHandler<EmptyData, EmptyData>(1, returnEmptyCallback);
    s.setMethodHandler<SendData, RecvData>(1, returnDataCallback);

    s.start();

    s.setMethodHandler<SendData, RecvData>(1, echoCallback);
    s.setMethodHandler<SendData, RecvData>(2, returnDataCallback);

    Client c(F::getPoll(), SOCKET_PATH);
    connectPeer(s, c);
    testEcho(c, 1);

    s.removeMethod(1);
    s.removeMethod(2);

    BOOST_CHECK_THROW(testEcho(c, 2), IPCException);
}

MULTI_FIXTURE_TEST_CASE(ClientAddRemoveMethod, F, ThreadedFixture, GlibFixture)
{
    Service s(F::getPoll(), SOCKET_PATH);
    Client c(F::getPoll(), SOCKET_PATH);
    c.setMethodHandler<EmptyData, EmptyData>(1, returnEmptyCallback);
    c.setMethodHandler<SendData, RecvData>(1, returnDataCallback);

    PeerID peerID = connectPeer(s, c);

    c.setMethodHandler<SendData, RecvData>(1, echoCallback);
    c.setMethodHandler<SendData, RecvData>(2, returnDataCallback);

    testEcho(s, 1, peerID);

    c.removeMethod(1);
    c.removeMethod(2);

    BOOST_CHECK_THROW(testEcho(s, 1, peerID), IPCException);
}

MULTI_FIXTURE_TEST_CASE(MethodResultGetPeerID, F, ThreadedFixture, GlibFixture)
{
    Service s(F::getPoll(), SOCKET_PATH);
    Client c(F::getPoll(), SOCKET_PATH);

    PeerID peerID = connectPeer(s, c);

    s.setMethodHandler<SendData, RecvData>(
        1,
        [&peerID](const PeerID,
                  std::shared_ptr<RecvData>&,
    MethodResult::Pointer methodResult) {
        methodResult->setVoid();
        BOOST_CHECK_EQUAL(peerID, methodResult->getPeerID());
        return HandlerExitCode::SUCCESS;
    }
    );

    std::shared_ptr<SendData> sentData(new SendData(32));
    std::shared_ptr<RecvData> recvData = c.callSync<SendData, RecvData>(1,
                                         sentData,
                                         TIMEOUT);
}

MULTI_FIXTURE_TEST_CASE(ServiceStartStop, F, ThreadedFixture, GlibFixture)
{
    Service s(F::getPoll(), SOCKET_PATH);

    s.setMethodHandler<SendData, RecvData>(1, returnDataCallback);

    s.start();
    s.stop();
    s.start();
    s.stop();

    s.start();
    s.start();
}

MULTI_FIXTURE_TEST_CASE(ClientStartStop, F, ThreadedFixture, GlibFixture)
{
    Service s(F::getPoll(), SOCKET_PATH);
    Client c(F::getPoll(), SOCKET_PATH);
    c.setMethodHandler<SendData, RecvData>(1, returnDataCallback);

    c.start();
    c.stop();
    c.start();
    c.stop();

    c.start();
    c.start();

    c.stop();
    c.stop();
}

MULTI_FIXTURE_TEST_CASE(SyncClientToServiceEcho, F, ThreadedFixture, GlibFixture)
{
    Service s(F::getPoll(), SOCKET_PATH);
    s.setMethodHandler<SendData, RecvData>(1, echoCallback);
    s.setMethodHandler<SendData, RecvData>(2, echoCallback);

    Client c(F::getPoll(), SOCKET_PATH);
    connectPeer(s, c);

    testEcho(c, 1);
    testEcho(c, 2);
}

MULTI_FIXTURE_TEST_CASE(Restart, F, ThreadedFixture, GlibFixture)
{
    Service s(F::getPoll(), SOCKET_PATH);
    s.setMethodHandler<SendData, RecvData>(1, echoCallback);
    s.start();
    s.setMethodHandler<SendData, RecvData>(2, echoCallback);

    Client c(F::getPoll(), SOCKET_PATH);
    c.start();
    testEcho(c, 1);
    testEcho(c, 2);

    c.stop();
    c.start();

    testEcho(c, 1);
    testEcho(c, 2);

    s.stop();
    s.start();

    BOOST_CHECK_THROW(testEcho(c, 2), IPCException);

    c.stop();
    c.start();

    testEcho(c, 1);
    testEcho(c, 2);
}

MULTI_FIXTURE_TEST_CASE(SyncServiceToClientEcho, F, ThreadedFixture, GlibFixture)
{
    Service s(F::getPoll(), SOCKET_PATH);
    Client c(F::getPoll(), SOCKET_PATH);
    c.setMethodHandler<SendData, RecvData>(1, echoCallback);
    PeerID peerID = connectPeer(s, c);

    std::shared_ptr<SendData> sentData(new SendData(56));
    std::shared_ptr<RecvData> recvData = s.callSync<SendData, RecvData>(1, peerID, sentData);
    BOOST_REQUIRE(recvData);
    BOOST_CHECK_EQUAL(recvData->intVal, sentData->intVal);
}

MULTI_FIXTURE_TEST_CASE(AsyncClientToServiceEcho, F, ThreadedFixture, GlibFixture)
{
    std::shared_ptr<SendData> sentData(new SendData(34));
    ValueLatch<std::shared_ptr<RecvData>> recvDataLatch;

    // Setup Service and Client
    Service s(F::getPoll(), SOCKET_PATH);
    s.setMethodHandler<SendData, RecvData>(1, echoCallback);
    s.start();
    Client c(F::getPoll(), SOCKET_PATH);
    c.start();

    //Async call
    auto dataBack = [&recvDataLatch](Result<RecvData> && r) {
        recvDataLatch.set(r.get());
    };
    c.callAsync<SendData, RecvData>(1, sentData, dataBack);

    // Wait for the response
    std::shared_ptr<RecvData> recvData(recvDataLatch.get(TIMEOUT));
    BOOST_CHECK_EQUAL(recvData->intVal, sentData->intVal);
}

MULTI_FIXTURE_TEST_CASE(AsyncServiceToClientEcho, F, ThreadedFixture, GlibFixture)
{
    std::shared_ptr<SendData> sentData(new SendData(56));
    ValueLatch<std::shared_ptr<RecvData>> recvDataLatch;

    Service s(F::getPoll(), SOCKET_PATH);
    Client c(F::getPoll(), SOCKET_PATH);
    c.setMethodHandler<SendData, RecvData>(1, echoCallback);
    PeerID peerID = connectPeer(s, c);

    // Async call
    auto dataBack = [&recvDataLatch](Result<RecvData> && r) {
        recvDataLatch.set(r.get());
    };

    s.callAsync<SendData, RecvData>(1, peerID, sentData, dataBack);

    // Wait for the response
    std::shared_ptr<RecvData> recvData(recvDataLatch.get(TIMEOUT));
    BOOST_CHECK_EQUAL(recvData->intVal, sentData->intVal);
}


MULTI_FIXTURE_TEST_CASE(SyncTimeout, F, ThreadedFixture, GlibFixture)
{
    Service s(F::getPoll(), SOCKET_PATH);
    s.setMethodHandler<SendData, RecvData>(1, longEchoCallback);

    Client c(F::getPoll(), SOCKET_PATH);
    connectPeer(s, c);

    std::shared_ptr<SendData> sentData(new SendData(78));
    BOOST_REQUIRE_THROW((c.callSync<SendData, RecvData>(1, sentData, TIMEOUT)), IPCException);
}

MULTI_FIXTURE_TEST_CASE(SerializationError, F, ThreadedFixture, GlibFixture)
{
    Service s(F::getPoll(), SOCKET_PATH);
    s.setMethodHandler<SendData, RecvData>(1, echoCallback);

    Client c(F::getPoll(), SOCKET_PATH);
    connectPeer(s, c);

    std::shared_ptr<ThrowOnAcceptData> throwingData(new ThrowOnAcceptData());

    BOOST_CHECK_THROW((c.callSync<ThrowOnAcceptData, RecvData>(1, throwingData)), IPCSerializationException);

}

MULTI_FIXTURE_TEST_CASE(ParseError, F, ThreadedFixture, GlibFixture)
{
    Service s(F::getPoll(), SOCKET_PATH);
    s.setMethodHandler<SendData, RecvData>(1, echoCallback);
    s.start();

    Client c(F::getPoll(), SOCKET_PATH);
    c.start();

    std::shared_ptr<SendData> sentData(new SendData(78));
    BOOST_CHECK_THROW((c.callSync<SendData, ThrowOnAcceptData>(1, sentData, 10000)), IPCParsingException);
}

MULTI_FIXTURE_TEST_CASE(DisconnectedPeerError, F, ThreadedFixture, GlibFixture)
{
    ValueLatch<Result<RecvData>> retStatusLatch;

    Service s(F::getPoll(), SOCKET_PATH);

    auto method = [](const PeerID, std::shared_ptr<ThrowOnAcceptData>&, MethodResult::Pointer methodResult) {
        auto resultData = std::make_shared<SendData>(1);
        methodResult->set<SendData>(resultData);
        return HandlerExitCode::SUCCESS;
    };

    // Method will throw during serialization and disconnect automatically
    s.setMethodHandler<SendData, ThrowOnAcceptData>(1, method);
    s.start();

    Client c(F::getPoll(), SOCKET_PATH);
    c.start();

    auto dataBack = [&retStatusLatch](Result<RecvData> && r) {
        retStatusLatch.set(std::move(r));
    };

    std::shared_ptr<SendData> sentData(new SendData(78));
    c.callAsync<SendData, RecvData>(1, sentData, dataBack);

    // Wait for the response
    Result<RecvData> result = retStatusLatch.get(TIMEOUT);

    // The disconnection might have happened:
    // - after sending the message (PEER_DISCONNECTED)
    // - during external serialization (SERIALIZATION_ERROR)
    BOOST_CHECK_THROW(result.get(), IPCException);
}


MULTI_FIXTURE_TEST_CASE(ReadTimeout, F, ThreadedFixture, GlibFixture)
{
    Service s(F::getPoll(), SOCKET_PATH);
    auto longEchoCallback = [](const PeerID, std::shared_ptr<RecvData>& data, MethodResult::Pointer methodResult) {
        auto resultData = std::make_shared<LongSendData>(data->intVal, LONG_OPERATION_TIME);
        methodResult->set<LongSendData>(resultData);
        return HandlerExitCode::SUCCESS;
    };
    s.setMethodHandler<LongSendData, RecvData>(1, longEchoCallback);

    Client c(F::getPoll(), SOCKET_PATH);
    connectPeer(s, c);

    // Test timeout on read
    std::shared_ptr<SendData> sentData(new SendData(334));
    BOOST_CHECK_THROW((c.callSync<SendData, RecvData>(1, sentData, TIMEOUT)), IPCException);
}


MULTI_FIXTURE_TEST_CASE(WriteTimeout, F, ThreadedFixture, GlibFixture)
{
    Service s(F::getPoll(), SOCKET_PATH);
    s.setMethodHandler<SendData, RecvData>(1, shortEchoCallback);
    s.start();

    Client c(F::getPoll(), SOCKET_PATH);
    c.start();

    // Test echo with a minimal timeout
    std::shared_ptr<LongSendData> sentDataA(new LongSendData(34, SHORT_OPERATION_TIME));
    std::shared_ptr<RecvData> recvData = c.callSync<LongSendData, RecvData>(1, sentDataA, TIMEOUT);
    BOOST_REQUIRE(recvData);
    BOOST_CHECK_EQUAL(recvData->intVal, sentDataA->intVal);

    // Test timeout on write
    std::shared_ptr<LongSendData> sentDataB(new LongSendData(34, LONG_OPERATION_TIME));
    BOOST_CHECK_THROW((c.callSync<LongSendData, RecvData>(1, sentDataB, TIMEOUT)), IPCTimeoutException);
}


MULTI_FIXTURE_TEST_CASE(AddSignalInRuntime, F, ThreadedFixture, GlibFixture)
{
    ValueLatch<std::shared_ptr<RecvData>> recvDataLatchA;
    ValueLatch<std::shared_ptr<RecvData>> recvDataLatchB;

    Service s(F::getPoll(), SOCKET_PATH);
    Client c(F::getPoll(), SOCKET_PATH);
    connectPeer(s, c);

    auto handlerA = [&recvDataLatchA](const PeerID, std::shared_ptr<RecvData>& data) {
        recvDataLatchA.set(data);
        return HandlerExitCode::SUCCESS;
    };

    auto handlerB = [&recvDataLatchB](const PeerID, std::shared_ptr<RecvData>& data) {
        recvDataLatchB.set(data);
        return HandlerExitCode::SUCCESS;
    };

    c.setSignalHandler<RecvData>(1, handlerA);
    c.setSignalHandler<RecvData>(2, handlerB);

    // Wait for the signals to propagate to the Service
    std::this_thread::sleep_for(std::chrono::milliseconds(2 * TIMEOUT));

    auto sendDataA = std::make_shared<SendData>(1);
    auto sendDataB = std::make_shared<SendData>(2);
    s.signal<SendData>(2, sendDataB);
    s.signal<SendData>(1, sendDataA);

    // Wait for the signals to arrive
    std::shared_ptr<RecvData> recvDataA(recvDataLatchA.get(TIMEOUT));
    std::shared_ptr<RecvData> recvDataB(recvDataLatchB.get(TIMEOUT));
    BOOST_CHECK_EQUAL(recvDataA->intVal, sendDataA->intVal);
    BOOST_CHECK_EQUAL(recvDataB->intVal, sendDataB->intVal);
}


MULTI_FIXTURE_TEST_CASE(AddSignalOffline, F, ThreadedFixture, GlibFixture)
{
    ValueLatch<std::shared_ptr<RecvData>> recvDataLatchA;
    ValueLatch<std::shared_ptr<RecvData>> recvDataLatchB;

    Service s(F::getPoll(), SOCKET_PATH);
    Client c(F::getPoll(), SOCKET_PATH);

    auto handlerA = [&recvDataLatchA](const PeerID, std::shared_ptr<RecvData>& data) {
        recvDataLatchA.set(data);
        return HandlerExitCode::SUCCESS;
    };

    auto handlerB = [&recvDataLatchB](const PeerID, std::shared_ptr<RecvData>& data) {
        recvDataLatchB.set(data);
        return HandlerExitCode::SUCCESS;
    };

    c.setSignalHandler<RecvData>(1, handlerA);
    c.setSignalHandler<RecvData>(2, handlerB);

    connectPeer(s, c);

    // Wait for the information about the signals to propagate
    std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT));

    auto sendDataA = std::make_shared<SendData>(1);
    auto sendDataB = std::make_shared<SendData>(2);
    s.signal<SendData>(2, sendDataB);
    s.signal<SendData>(1, sendDataA);

    // Wait for the signals to arrive
    std::shared_ptr<RecvData> recvDataA(recvDataLatchA.get(TIMEOUT));
    std::shared_ptr<RecvData> recvDataB(recvDataLatchB.get(TIMEOUT));
    BOOST_CHECK_EQUAL(recvDataA->intVal, sendDataA->intVal);
    BOOST_CHECK_EQUAL(recvDataB->intVal, sendDataB->intVal);
}

MULTI_FIXTURE_TEST_CASE(UsersError, F, ThreadedFixture, GlibFixture)
{
    const int TEST_ERROR_CODE = -234;
    const std::string TEST_ERROR_MESSAGE = "Ay, caramba!";

    Service s(F::getPoll(), SOCKET_PATH);
    Client c(F::getPoll(), SOCKET_PATH);
    auto clientID = connectPeer(s, c);

    auto throwingMethodHandler = [&](const PeerID, std::shared_ptr<RecvData>&, MethodResult::Pointer) {
        throw IPCUserException(TEST_ERROR_CODE, TEST_ERROR_MESSAGE);
        return HandlerExitCode::SUCCESS;
    };

    auto sendErrorMethodHandler = [&](const PeerID, std::shared_ptr<RecvData>&, MethodResult::Pointer methodResult) {
        methodResult->setError(TEST_ERROR_CODE, TEST_ERROR_MESSAGE);
        return HandlerExitCode::SUCCESS;
    };

    s.setMethodHandler<SendData, RecvData>(1, throwingMethodHandler);
    s.setMethodHandler<SendData, RecvData>(2, sendErrorMethodHandler);
    c.setMethodHandler<SendData, RecvData>(1, throwingMethodHandler);
    c.setMethodHandler<SendData, RecvData>(2, sendErrorMethodHandler);

    std::shared_ptr<SendData> sentData(new SendData(78));

    auto hasProperData = [&](const IPCUserException & e) {
        return e.getCode() == TEST_ERROR_CODE && e.what() == TEST_ERROR_MESSAGE;
    };

    BOOST_CHECK_EXCEPTION((c.callSync<SendData, RecvData>(1, sentData, TIMEOUT)), IPCUserException, hasProperData);
    BOOST_CHECK_EXCEPTION((s.callSync<SendData, RecvData>(1, clientID, sentData, TIMEOUT)), IPCUserException, hasProperData);
    BOOST_CHECK_EXCEPTION((c.callSync<SendData, RecvData>(2, sentData, TIMEOUT)), IPCUserException, hasProperData);
    BOOST_CHECK_EXCEPTION((s.callSync<SendData, RecvData>(2, clientID, sentData, TIMEOUT)), IPCUserException, hasProperData);
}

MULTI_FIXTURE_TEST_CASE(AsyncResult, F, ThreadedFixture, GlibFixture)
{
    const int TEST_ERROR_CODE = -567;
    const std::string TEST_ERROR_MESSAGE = "Ooo jooo!";

    Service s(F::getPoll(), SOCKET_PATH);
    Client c(F::getPoll(), SOCKET_PATH);
    auto clientID = connectPeer(s, c);

    auto errorMethodHandler = [&](const PeerID, std::shared_ptr<RecvData>&, MethodResult::Pointer methodResult) {
        std::async(std::launch::async, [&, methodResult] {
            std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_OPERATION_TIME));
            methodResult->setError(TEST_ERROR_CODE, TEST_ERROR_MESSAGE);
        });
        return HandlerExitCode::SUCCESS;
    };

    auto voidMethodHandler = [&](const PeerID, std::shared_ptr<RecvData>&, MethodResult::Pointer methodResult) {
        std::async(std::launch::async, [methodResult] {
            std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_OPERATION_TIME));
            methodResult->setVoid();
        });
        return HandlerExitCode::SUCCESS;
    };

    auto dataMethodHandler = [&](const PeerID, std::shared_ptr<RecvData>& data, MethodResult::Pointer methodResult) {
        std::async(std::launch::async, [data, methodResult] {
            std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_OPERATION_TIME));
            methodResult->set(data);
        });
        return HandlerExitCode::SUCCESS;
    };

    s.setMethodHandler<SendData, RecvData>(1, errorMethodHandler);
    s.setMethodHandler<EmptyData, RecvData>(2, voidMethodHandler);
    s.setMethodHandler<SendData, RecvData>(3, dataMethodHandler);
    c.setMethodHandler<SendData, RecvData>(1, errorMethodHandler);
    c.setMethodHandler<EmptyData, RecvData>(2, voidMethodHandler);
    c.setMethodHandler<SendData, RecvData>(3, dataMethodHandler);

    std::shared_ptr<SendData> sentData(new SendData(90));

    auto hasProperData = [&](const IPCUserException & e) {
        return e.getCode() == TEST_ERROR_CODE && e.what() == TEST_ERROR_MESSAGE;
    };

    BOOST_CHECK_EXCEPTION((s.callSync<SendData, RecvData>(1, clientID, sentData, TIMEOUT)), IPCUserException, hasProperData);
    BOOST_CHECK_EXCEPTION((c.callSync<SendData, RecvData>(1, sentData, TIMEOUT)), IPCUserException, hasProperData);

    BOOST_CHECK_NO_THROW((s.callSync<SendData, EmptyData>(2, clientID, sentData, TIMEOUT)));
    BOOST_CHECK_NO_THROW((c.callSync<SendData, EmptyData>(2, sentData, TIMEOUT)));

    std::shared_ptr<RecvData> recvData;
    recvData = s.callSync<SendData, RecvData>(3, clientID, sentData, TIMEOUT);
    BOOST_CHECK_EQUAL(recvData->intVal, sentData->intVal);
    recvData = c.callSync<SendData, RecvData>(3, sentData, TIMEOUT);
    BOOST_CHECK_EQUAL(recvData->intVal, sentData->intVal);
}

MULTI_FIXTURE_TEST_CASE(MixOperations, F, ThreadedFixture, GlibFixture)
{
    utils::Latch l;

    auto signalHandler = [&l](const PeerID, std::shared_ptr<RecvData>&) {
        l.set();
        return HandlerExitCode::SUCCESS;
    };

    Service s(F::getPoll(), SOCKET_PATH);
    s.setMethodHandler<SendData, RecvData>(1, echoCallback);

    Client c(F::getPoll(), SOCKET_PATH);
    s.setSignalHandler<RecvData>(2, signalHandler);

    connectPeer(s, c);

    testEcho(c, 1);

    auto data = std::make_shared<SendData>(1);
    c.signal<SendData>(2, data);

    BOOST_CHECK(l.wait(TIMEOUT));
}

MULTI_FIXTURE_TEST_CASE(FDSendReceive, F, ThreadedFixture, GlibFixture)
{
    const char DATA[] = "Content of the file";
    {
        // Fill the file
        fs::remove(TEST_FILE);
        std::ofstream file(TEST_FILE);
        file << DATA;
        file.close();
    }

    auto methodHandler = [&](const PeerID, std::shared_ptr<EmptyData>&, MethodResult::Pointer methodResult) {
        int fd = ::open(TEST_FILE.c_str(), O_RDONLY);
        auto returnData = std::make_shared<FDData>(fd);
        methodResult->set(returnData);
        return HandlerExitCode::SUCCESS;
    };

    Service s(F::getPoll(), SOCKET_PATH);
    s.setMethodHandler<FDData, EmptyData>(1, methodHandler);

    Client c(F::getPoll(), SOCKET_PATH);
    connectPeer(s, c);

    std::shared_ptr<FDData> fdData;
    std::shared_ptr<EmptyData> sentData(new EmptyData());
    fdData = c.callSync<EmptyData, FDData>(1, sentData, TIMEOUT);

    // Use the file descriptor
    char buffer[sizeof(DATA)];
    BOOST_REQUIRE(::read(fdData->fd.value, buffer, sizeof(buffer))>0);
    BOOST_REQUIRE(strncmp(DATA, buffer, strlen(DATA))==0);
    ::close(fdData->fd.value);
}

MULTI_FIXTURE_TEST_CASE(OneShotMethodHandler, F, ThreadedFixture, GlibFixture)
{
    auto methodHandler = [&](const PeerID, std::shared_ptr<EmptyData>&, MethodResult::Pointer methodResult) {
        methodResult->setVoid();
        return HandlerExitCode::REMOVE_HANDLER;
    };

    Service s(F::getPoll(), SOCKET_PATH);
    s.setMethodHandler<EmptyData, EmptyData>(1, methodHandler);

    Client c(F::getPoll(), SOCKET_PATH);
    connectPeer(s, c);

    std::shared_ptr<EmptyData> sentData(new EmptyData());
    c.callSync<EmptyData, EmptyData>(1, sentData, TIMEOUT);
    BOOST_REQUIRE_THROW((c.callSync<EmptyData, EmptyData>(1, sentData, TIMEOUT)), IPCNaughtyPeerException);
}

MULTI_FIXTURE_TEST_CASE(OneShotSignalHandler, F, ThreadedFixture, GlibFixture)
{
    utils::Latch latch;

    auto signalHandler = [&latch](const PeerID, std::shared_ptr<EmptyData>&) {
        latch.set();
        return HandlerExitCode::REMOVE_HANDLER;
    };

    Service s(F::getPoll(), SOCKET_PATH);
    s.setSignalHandler<EmptyData>(1, signalHandler);

    Client c(F::getPoll(), SOCKET_PATH);
    connectPeer(s, c);

    // Wait for the signals to propagate to the Service
    std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT));

    std::shared_ptr<EmptyData> sentData(new EmptyData());
    c.signal<EmptyData>(1, sentData);

    // Wait for the signal
    BOOST_REQUIRE(latch.wait(TIMEOUT));
    BOOST_REQUIRE(!s.isHandled(1));
}

BOOST_AUTO_TEST_CASE(ConnectionLimit)
{
    const unsigned oldLimit = utils::getMaxFDNumber();
    const unsigned newLimit = 128;
    ScopedDir scopedDir(TEST_DIR);

    Channel c;

    const pid_t chpid = ::fork();
    BOOST_CHECK_NE(chpid, -1);

    if (chpid) {
        // Setup Service
        ThreadDispatcher td;
        Service s(td.getPoll(), SOCKET_PATH);
        s.setMethodHandler<SendData, RecvData>(1, echoCallback);
        s.start();

        c.setLeft();
        try {
            // inform the Client
            c.write(true);
        } catch (...) {
            kill(chpid, 9);
            throw;
        }

        int status;
        BOOST_CHECK_EQUAL(::waitpid(chpid, &status, 0), chpid);
        BOOST_CHECK_EQUAL(status, EXIT_SUCCESS);
    } else {
        int ret = EXIT_FAILURE;

        c.setRight();
        try {
            // wait for the Service
            c.read<char>();
        } catch(...) {
            ::_exit(EXIT_FAILURE);
        }

        ThreadDispatcher td;
        std::list<Client> clients;

        utils::setMaxFDNumber(newLimit);

        // Setup Clients
        try {
            for (unsigned i = 0; i < 2 * newLimit; ++i) {
                clients.emplace_back(td.getPoll(), SOCKET_PATH);
                clients.back().start();
            }
        } catch (const EventFDException& e) {
            ret = EXIT_SUCCESS;
        } catch (const IPCSocketException& e) {
            if (e.getCode() == EMFILE) {
                ret = EXIT_SUCCESS;
            }
        }

        utils::setMaxFDNumber(oldLimit);

        ::_exit(ret);
    }
}

BOOST_AUTO_TEST_SUITE_END()
