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

// TODO: Test connection limit
// TODO: Refactor tests - function for setting up env


#include "config.hpp"
#include "ut.hpp"

#include "ipc/service.hpp"
#include "ipc/client.hpp"
#include "ipc/types.hpp"
#include "ipc/result.hpp"
#include "utils/glib-loop.hpp"
#include "utils/latch.hpp"
#include "utils/value-latch.hpp"
#include "utils/scoped-dir.hpp"

#include "config/fields.hpp"
#include "logger/logger.hpp"

#include <atomic>
#include <string>
#include <thread>
#include <chrono>
#include <utility>
#include <future>

using namespace vasum;
using namespace vasum::ipc;
using namespace vasum::utils;
using namespace std::placeholders;

namespace {

// Timeout for sending one message
const int TIMEOUT = 1000 /*ms*/;

// Time that won't cause "TIMEOUT" methods to throw
const int SHORT_OPERATION_TIME = TIMEOUT / 100;

// Time that will cause "TIMEOUT" methods to throw
const int LONG_OPERATION_TIME = 1000 + TIMEOUT;

const std::string TEST_DIR = "/tmp/ut-ipc";
const std::string SOCKET_PATH = TEST_DIR + "/test.socket";

struct Fixture {
    ScopedDir mTestPathGuard;
    std::string SOCKET_PATH;

    Fixture()
        : mTestPathGuard(TEST_DIR)
    {
    }
};

struct SendData {
    int intVal;
    SendData(int i): intVal(i) {}

    CONFIG_REGISTER
    (
        intVal
    )
};

struct RecvData {
    int intVal;
    RecvData(): intVal(-1) {}

    CONFIG_REGISTER
    (
        intVal
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
    CONFIG_REGISTER_EMPTY
};

struct ThrowOnAcceptData {
    template<typename Visitor>
    void accept(Visitor)
    {
        throw std::runtime_error("intentional failure in accept");
    }
    template<typename Visitor>
    void accept(Visitor) const
    {
        throw std::runtime_error("intentional failure in accept const");
    }
};

void returnEmptyCallback(const PeerID,
                         std::shared_ptr<EmptyData>&,
                         MethodResult::Pointer methodResult)
{
    methodResult->setVoid();
}

void returnDataCallback(const PeerID,
                        std::shared_ptr<RecvData>&,
                        MethodResult::Pointer methodResult)
{
    auto returnData = std::make_shared<SendData>(1);
    methodResult->set(returnData);
}

void echoCallback(const PeerID,
                  std::shared_ptr<RecvData>& data,
                  MethodResult::Pointer methodResult)
{
    auto returnData = std::make_shared<SendData>(data->intVal);
    methodResult->set(returnData);
}

void longEchoCallback(const PeerID,
                      std::shared_ptr<RecvData>& data,
                      MethodResult::Pointer methodResult)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(LONG_OPERATION_TIME));
    auto returnData = std::make_shared<SendData>(data->intVal);
    methodResult->set(returnData);
}

PeerID connect(Service& s, Client& c, bool isServiceGlib = false, bool isClientGlib = false)
{
    // Connects the Client to the Service and returns Clients PeerID
    ValueLatch<PeerID> peerIDLatch;
    auto newPeerCallback = [&peerIDLatch](const PeerID newID, const FileDescriptor) {
        peerIDLatch.set(newID);
    };

    s.setNewPeerCallback(newPeerCallback);

    if (!s.isStarted()) {
        s.start(isServiceGlib);
    }

    c.start(isClientGlib);

    PeerID peerID = peerIDLatch.get(TIMEOUT);
    s.setNewPeerCallback(nullptr);
    BOOST_REQUIRE_NE(peerID, 0);
    return peerID;
}

PeerID connectServiceGSource(Service& s, Client& c)
{
    return connect(s, c, true, false);
}

PeerID connectClientGSource(Service& s, Client& c)
{
    return connect(s, c, false, true);
}

void testEcho(Client& c, const MethodID methodID)
{
    std::shared_ptr<SendData> sentData(new SendData(34));
    std::shared_ptr<RecvData> recvData = c.callSync<SendData, RecvData>(methodID, sentData, TIMEOUT);
    BOOST_REQUIRE(recvData);
    BOOST_CHECK_EQUAL(recvData->intVal, sentData->intVal);
}

void testEcho(Service& s, const MethodID methodID, const PeerID peerID)
{
    std::shared_ptr<SendData> sentData(new SendData(56));
    std::shared_ptr<RecvData> recvData = s.callSync<SendData, RecvData>(methodID, peerID, sentData, TIMEOUT);
    BOOST_REQUIRE(recvData);
    BOOST_CHECK_EQUAL(recvData->intVal, sentData->intVal);
}

} // namespace


BOOST_FIXTURE_TEST_SUITE(IPCSuite, Fixture)

BOOST_AUTO_TEST_CASE(ConstructorDestructor)
{
    Service s(SOCKET_PATH);
    Client c(SOCKET_PATH);
}

BOOST_AUTO_TEST_CASE(ServiceAddRemoveMethod)
{
    Service s(SOCKET_PATH);
    s.setMethodHandler<EmptyData, EmptyData>(1, returnEmptyCallback);
    s.setMethodHandler<SendData, RecvData>(1, returnDataCallback);

    s.start();

    s.setMethodHandler<SendData, RecvData>(1, echoCallback);
    s.setMethodHandler<SendData, RecvData>(2, returnDataCallback);

    Client c(SOCKET_PATH);
    connect(s, c);
    testEcho(c, 1);

    s.removeMethod(1);
    s.removeMethod(2);

    BOOST_CHECK_THROW(testEcho(c, 2), IPCException);
}

BOOST_AUTO_TEST_CASE(ClientAddRemoveMethod)
{
    Service s(SOCKET_PATH);
    Client c(SOCKET_PATH);
    c.setMethodHandler<EmptyData, EmptyData>(1, returnEmptyCallback);
    c.setMethodHandler<SendData, RecvData>(1, returnDataCallback);

    PeerID peerID = connect(s, c);

    c.setMethodHandler<SendData, RecvData>(1, echoCallback);
    c.setMethodHandler<SendData, RecvData>(2, returnDataCallback);

    testEcho(s, 1, peerID);

    c.removeMethod(1);
    c.removeMethod(2);

    BOOST_CHECK_THROW(testEcho(s, 1, peerID), IPCException);
}

BOOST_AUTO_TEST_CASE(ServiceStartStop)
{
    Service s(SOCKET_PATH);

    s.setMethodHandler<SendData, RecvData>(1, returnDataCallback);

    s.start();
    s.stop();
    s.start();
    s.stop();

    s.start();
    s.start();
}

BOOST_AUTO_TEST_CASE(ClientStartStop)
{
    Service s(SOCKET_PATH);
    Client c(SOCKET_PATH);
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

BOOST_AUTO_TEST_CASE(SyncClientToServiceEcho)
{
    Service s(SOCKET_PATH);
    s.setMethodHandler<SendData, RecvData>(1, echoCallback);
    s.setMethodHandler<SendData, RecvData>(2, echoCallback);

    Client c(SOCKET_PATH);
    connect(s, c);

    testEcho(c, 1);
    testEcho(c, 2);
}

BOOST_AUTO_TEST_CASE(Restart)
{
    Service s(SOCKET_PATH);
    s.setMethodHandler<SendData, RecvData>(1, echoCallback);
    s.start();
    s.setMethodHandler<SendData, RecvData>(2, echoCallback);

    Client c(SOCKET_PATH);
    c.start();
    testEcho(c, 1);
    testEcho(c, 2);

    c.stop();
    c.start();

    testEcho(c, 1);
    testEcho(c, 2);

    s.stop();
    s.start();

    testEcho(c, 1);
    testEcho(c, 2);
}

BOOST_AUTO_TEST_CASE(SyncServiceToClientEcho)
{
    Service s(SOCKET_PATH);
    Client c(SOCKET_PATH);
    c.setMethodHandler<SendData, RecvData>(1, echoCallback);
    PeerID peerID = connect(s, c);

    std::shared_ptr<SendData> sentData(new SendData(56));
    std::shared_ptr<RecvData> recvData = s.callSync<SendData, RecvData>(1, peerID, sentData);
    BOOST_REQUIRE(recvData);
    BOOST_CHECK_EQUAL(recvData->intVal, sentData->intVal);
}

BOOST_AUTO_TEST_CASE(AsyncClientToServiceEcho)
{
    std::shared_ptr<SendData> sentData(new SendData(34));
    ValueLatch<std::shared_ptr<RecvData>> recvDataLatch;

    // Setup Service and Client
    Service s(SOCKET_PATH);
    s.setMethodHandler<SendData, RecvData>(1, echoCallback);
    s.start();
    Client c(SOCKET_PATH);
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

BOOST_AUTO_TEST_CASE(AsyncServiceToClientEcho)
{
    std::shared_ptr<SendData> sentData(new SendData(56));
    ValueLatch<std::shared_ptr<RecvData>> recvDataLatch;

    Service s(SOCKET_PATH);
    Client c(SOCKET_PATH);
    c.setMethodHandler<SendData, RecvData>(1, echoCallback);
    PeerID peerID = connect(s, c);

    // Async call
    auto dataBack = [&recvDataLatch](Result<RecvData> && r) {
        recvDataLatch.set(r.get());
    };

    s.callAsync<SendData, RecvData>(1, peerID, sentData, dataBack);

    // Wait for the response
    std::shared_ptr<RecvData> recvData(recvDataLatch.get(TIMEOUT));
    BOOST_CHECK_EQUAL(recvData->intVal, sentData->intVal);
}


BOOST_AUTO_TEST_CASE(SyncTimeout)
{
    Service s(SOCKET_PATH);
    s.setMethodHandler<SendData, RecvData>(1, longEchoCallback);

    Client c(SOCKET_PATH);
    connect(s, c);

    std::shared_ptr<SendData> sentData(new SendData(78));
    BOOST_REQUIRE_THROW((c.callSync<SendData, RecvData>(1, sentData, TIMEOUT)), IPCException);
}

BOOST_AUTO_TEST_CASE(SerializationError)
{
    Service s(SOCKET_PATH);
    s.setMethodHandler<SendData, RecvData>(1, echoCallback);

    Client c(SOCKET_PATH);
    connect(s, c);

    std::shared_ptr<ThrowOnAcceptData> throwingData(new ThrowOnAcceptData());

    BOOST_CHECK_THROW((c.callSync<ThrowOnAcceptData, RecvData>(1, throwingData)), IPCSerializationException);

}

BOOST_AUTO_TEST_CASE(ParseError)
{
    Service s(SOCKET_PATH);
    s.setMethodHandler<SendData, RecvData>(1, echoCallback);
    s.start();

    Client c(SOCKET_PATH);
    c.start();

    std::shared_ptr<SendData> sentData(new SendData(78));
    BOOST_CHECK_THROW((c.callSync<SendData, ThrowOnAcceptData>(1, sentData, 10000)), IPCParsingException);
}

BOOST_AUTO_TEST_CASE(DisconnectedPeerError)
{
    ValueLatch<Result<RecvData>> retStatusLatch;
    Service s(SOCKET_PATH);

    auto method = [](const PeerID, std::shared_ptr<ThrowOnAcceptData>&, MethodResult::Pointer methodResult) {
        auto resultData = std::make_shared<SendData>(1);
        methodResult->set<SendData>(resultData);
    };

    // Method will throw during serialization and disconnect automatically
    s.setMethodHandler<SendData, ThrowOnAcceptData>(1, method);
    s.start();

    Client c(SOCKET_PATH);
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


BOOST_AUTO_TEST_CASE(ReadTimeout)
{
    Service s(SOCKET_PATH);
    auto longEchoCallback = [](const PeerID, std::shared_ptr<RecvData>& data, MethodResult::Pointer methodResult) {
        auto resultData = std::make_shared<LongSendData>(data->intVal, LONG_OPERATION_TIME);
        methodResult->set<LongSendData>(resultData);
    };
    s.setMethodHandler<LongSendData, RecvData>(1, longEchoCallback);

    Client c(SOCKET_PATH);
    connect(s, c);

    // Test timeout on read
    std::shared_ptr<SendData> sentData(new SendData(334));
    BOOST_CHECK_THROW((c.callSync<SendData, RecvData>(1, sentData, TIMEOUT)), IPCException);
}


BOOST_AUTO_TEST_CASE(WriteTimeout)
{
    Service s(SOCKET_PATH);
    s.setMethodHandler<SendData, RecvData>(1, echoCallback);
    s.start();

    Client c(SOCKET_PATH);
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


BOOST_AUTO_TEST_CASE(AddSignalInRuntime)
{
    ValueLatch<std::shared_ptr<RecvData>> recvDataLatchA;
    ValueLatch<std::shared_ptr<RecvData>> recvDataLatchB;

    Service s(SOCKET_PATH);
    Client c(SOCKET_PATH);
    connect(s, c);

    auto handlerA = [&recvDataLatchA](const PeerID, std::shared_ptr<RecvData>& data) {
        recvDataLatchA.set(data);
    };

    auto handlerB = [&recvDataLatchB](const PeerID, std::shared_ptr<RecvData>& data) {
        recvDataLatchB.set(data);
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


BOOST_AUTO_TEST_CASE(AddSignalOffline)
{
    ValueLatch<std::shared_ptr<RecvData>> recvDataLatchA;
    ValueLatch<std::shared_ptr<RecvData>> recvDataLatchB;

    Service s(SOCKET_PATH);
    Client c(SOCKET_PATH);

    auto handlerA = [&recvDataLatchA](const PeerID, std::shared_ptr<RecvData>& data) {
        recvDataLatchA.set(data);
    };

    auto handlerB = [&recvDataLatchB](const PeerID, std::shared_ptr<RecvData>& data) {
        recvDataLatchB.set(data);
    };

    c.setSignalHandler<RecvData>(1, handlerA);
    c.setSignalHandler<RecvData>(2, handlerB);

    connect(s, c);

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


BOOST_AUTO_TEST_CASE(ServiceGSource)
{
    utils::Latch l;
    ScopedGlibLoop loop;

    auto signalHandler = [&l](const PeerID, std::shared_ptr<RecvData>&) {
        l.set();
    };

    IPCGSource::Pointer serviceGSource;
    Service s(SOCKET_PATH);
    s.setMethodHandler<SendData, RecvData>(1, echoCallback);

    Client c(SOCKET_PATH);
    s.setSignalHandler<RecvData>(2, signalHandler);

    connectServiceGSource(s, c);

    testEcho(c, 1);

    auto data = std::make_shared<SendData>(1);
    c.signal<SendData>(2, data);

    BOOST_CHECK(l.wait(TIMEOUT));
}


BOOST_AUTO_TEST_CASE(ClientGSource)
{
    utils::Latch l;
    ScopedGlibLoop loop;

    auto signalHandler = [&l](const PeerID, std::shared_ptr<RecvData>&) {
        l.set();
    };

    Service s(SOCKET_PATH);
    s.start();

    IPCGSource::Pointer clientGSource;
    Client c(SOCKET_PATH);
    c.setMethodHandler<SendData, RecvData>(1, echoCallback);
    c.setSignalHandler<RecvData>(2, signalHandler);

    PeerID peerID = connectClientGSource(s, c);

    testEcho(s, 1, peerID);

    auto data = std::make_shared<SendData>(1);
    s.signal<SendData>(2, data);

    BOOST_CHECK(l.wait(TIMEOUT));
}

BOOST_AUTO_TEST_CASE(UsersError)
{
    const int TEST_ERROR_CODE = -234;
    const std::string TEST_ERROR_MESSAGE = "Ay, caramba!";

    Service s(SOCKET_PATH);
    Client c(SOCKET_PATH);
    auto clientID = connect(s, c);

    auto throwingMethodHandler = [&](const PeerID, std::shared_ptr<RecvData>&, MethodResult::Pointer) {
        throw IPCUserException(TEST_ERROR_CODE, TEST_ERROR_MESSAGE);
    };

    auto sendErrorMethodHandler = [&](const PeerID, std::shared_ptr<RecvData>&, MethodResult::Pointer methodResult) {
        methodResult->setError(TEST_ERROR_CODE, TEST_ERROR_MESSAGE);
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

BOOST_AUTO_TEST_CASE(AsyncResult)
{
    const int TEST_ERROR_CODE = -567;
    const std::string TEST_ERROR_MESSAGE = "Ooo jooo!";

    Service s(SOCKET_PATH);
    Client c(SOCKET_PATH);
    auto clientID = connect(s, c);

    auto errorMethodHandler = [&](const PeerID, std::shared_ptr<RecvData>&, MethodResult::Pointer methodResult) {
        std::async(std::launch::async, [&, methodResult] {
            std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_OPERATION_TIME));
            methodResult->setError(TEST_ERROR_CODE, TEST_ERROR_MESSAGE);
        });
    };

    auto voidMethodHandler = [&](const PeerID, std::shared_ptr<RecvData>&, MethodResult::Pointer methodResult) {
        std::async(std::launch::async, [methodResult] {
            std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_OPERATION_TIME));
            methodResult->setVoid();
        });
    };

    auto dataMethodHandler = [&](const PeerID, std::shared_ptr<RecvData>& data, MethodResult::Pointer methodResult) {
        std::async(std::launch::async, [data, methodResult] {
            std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_OPERATION_TIME));
            methodResult->set(data);
        });
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

// BOOST_AUTO_TEST_CASE(ConnectionLimitTest)
// {
//     unsigned oldLimit = ipc::getMaxFDNumber();
//     ipc::setMaxFDNumber(50);

//     // Setup Service and many Clients
//     Service s(SOCKET_PATH);
//     s.setMethodHandler<SendData, RecvData>(1, echoCallback);
//     s.start();

//     std::list<Client> clients;
//     for (int i = 0; i < 100; ++i) {
//         try {
//             clients.push_back(Client(SOCKET_PATH));
//             clients.back().start();
//         } catch (...) {}
//     }

//     unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
//     std::mt19937 generator(seed);
//     for (auto it = clients.begin(); it != clients.end(); ++it) {
//         try {
//             std::shared_ptr<SendData> sentData(new SendData(generator()));
//             std::shared_ptr<RecvData> recvData = it->callSync<SendData, RecvData>(1, sentData);
//             BOOST_CHECK_EQUAL(recvData->intVal, sentData->intVal);
//         } catch (...) {}
//     }

//     ipc::setMaxFDNumber(oldLimit);
// }



BOOST_AUTO_TEST_SUITE_END()
