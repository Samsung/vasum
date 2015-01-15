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
// TODO: Callback wrapper that waits till the callback is called


#include "config.hpp"
#include "ut.hpp"

#include "ipc/service.hpp"
#include "ipc/client.hpp"
#include "ipc/ipc-gsource.hpp"
#include "ipc/types.hpp"
#include "utils/glib-loop.hpp"
#include "utils/latch.hpp"

#include "config/fields.hpp"
#include "logger/logger.hpp"

#include <atomic>
#include <string>
#include <thread>
#include <chrono>
#include <utility>
#include <boost/filesystem.hpp>

using namespace vasum;
using namespace vasum::ipc;
using namespace vasum::utils;
using namespace std::placeholders;
namespace fs = boost::filesystem;

namespace {

// Timeout for sending one message
const int TIMEOUT = 1000 /*ms*/;

// Time that won't cause "TIMEOUT" methods to throw
const int SHORT_OPERATION_TIME = TIMEOUT / 100;

// Time that will cause "TIMEOUT" methods to throw
const int LONG_OPERATION_TIME = 1000 + TIMEOUT;

struct Fixture {
    std::string socketPath;

    Fixture()
        : socketPath(fs::unique_path("/tmp/ipc-%%%%.socket").string())
    {
    }
    ~Fixture()
    {
        fs::remove(socketPath);
    }
};

struct SendData {
    int intVal;
    SendData(int i = 0): intVal(i) {}

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

std::shared_ptr<EmptyData> returnEmptyCallback(const FileDescriptor, std::shared_ptr<EmptyData>&)
{
    return std::shared_ptr<EmptyData>(new EmptyData());
}

std::shared_ptr<SendData> returnDataCallback(const FileDescriptor, std::shared_ptr<SendData>&)
{
    return std::shared_ptr<SendData>(new SendData(1));
}

std::shared_ptr<SendData> echoCallback(const FileDescriptor, std::shared_ptr<SendData>& data)
{
    return data;
}

std::shared_ptr<SendData> longEchoCallback(const FileDescriptor, std::shared_ptr<SendData>& data)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(LONG_OPERATION_TIME));
    return data;
}

FileDescriptor connect(Service& s, Client& c)
{
    // Connects the Client to the Service and returns Clients FileDescriptor
    std::mutex mutex;
    std::condition_variable cv;

    FileDescriptor peerFD = 0;
    auto newPeerCallback = [&cv, &peerFD, &mutex](const FileDescriptor newFD) {
        std::unique_lock<std::mutex> lock(mutex);
        peerFD = newFD;
        cv.notify_all();
    };

    // TODO: On timeout remove the callback
    s.setNewPeerCallback(newPeerCallback);

    if (!s.isStarted()) {
        s.start();
    }

    c.start();


    std::unique_lock<std::mutex> lock(mutex);
    BOOST_REQUIRE(cv.wait_for(lock, std::chrono::milliseconds(3 * TIMEOUT), [&peerFD]() {
        return peerFD != 0;
    }));

    return peerFD;
}



#if GLIB_CHECK_VERSION(2,36,0)

std::pair<FileDescriptor, IPCGSource::Pointer> connectServiceGSource(Service& s, Client& c)
{
    std::mutex mutex;
    std::condition_variable cv;

    FileDescriptor peerFD = 0;
    IPCGSource::Pointer ipcGSourcePtr = IPCGSource::create(s.getFDs(), std::bind(&Service::handle, &s, _1, _2));

    auto newPeerCallback = [&cv, &peerFD, &mutex, ipcGSourcePtr](const FileDescriptor newFD) {
        if (ipcGSourcePtr) {
            //TODO: Remove this if
            ipcGSourcePtr->addFD(newFD);
        }
        std::unique_lock<std::mutex> lock(mutex);
        peerFD = newFD;
        cv.notify_all();
    };


    // TODO: On timeout remove the callback
    s.setNewPeerCallback(newPeerCallback);
    s.setRemovedPeerCallback(std::bind(&IPCGSource::removeFD, ipcGSourcePtr, _1));
    s.start(true);
    // Service starts to process
    ipcGSourcePtr->attach();

    c.start();

    std::unique_lock<std::mutex> lock(mutex);
    BOOST_REQUIRE(cv.wait_for(lock, std::chrono::milliseconds(3 * TIMEOUT), [&peerFD]() {
        return peerFD != 0;
    }));

    return std::make_pair(peerFD, ipcGSourcePtr);
}

std::pair<FileDescriptor, IPCGSource::Pointer> connectClientGSource(Service& s, Client& c)
{
    // Connects the Client to the Service and returns Clients FileDescriptor
    std::mutex mutex;
    std::condition_variable cv;

    FileDescriptor peerFD = 0;
    auto newPeerCallback = [&cv, &peerFD, &mutex](const FileDescriptor newFD) {
        std::unique_lock<std::mutex> lock(mutex);
        peerFD = newFD;
        cv.notify_all();
    };
    // TODO: On timeout remove the callback
    s.setNewPeerCallback(newPeerCallback);

    if (!s.isStarted()) {
        // Service starts to process
        s.start();
    }


    c.start(true);
    IPCGSource::Pointer ipcGSourcePtr = IPCGSource::create(c.getFDs(),
                                                           std::bind(&Client::handle, &c, _1, _2));

    ipcGSourcePtr->attach();

    std::unique_lock<std::mutex> lock(mutex);
    BOOST_REQUIRE(cv.wait_for(lock, std::chrono::milliseconds(3 * TIMEOUT), [&peerFD]() {
        return peerFD != 0;
    }));

    return std::make_pair(peerFD, ipcGSourcePtr);
}

#endif // GLIB_CHECK_VERSION


void testEcho(Client& c, const MethodID methodID)
{
    std::shared_ptr<SendData> sentData(new SendData(34));
    std::shared_ptr<SendData> recvData = c.callSync<SendData, SendData>(methodID, sentData, TIMEOUT);
    BOOST_CHECK_EQUAL(recvData->intVal, sentData->intVal);
}

void testEcho(Service& s, const MethodID methodID, const FileDescriptor peerFD)
{
    std::shared_ptr<SendData> sentData(new SendData(56));
    std::shared_ptr<SendData> recvData = s.callSync<SendData, SendData>(methodID, peerFD, sentData, TIMEOUT);
    BOOST_CHECK_EQUAL(recvData->intVal, sentData->intVal);
}

} // namespace


BOOST_FIXTURE_TEST_SUITE(IPCSuite, Fixture)

BOOST_AUTO_TEST_CASE(ConstructorDestructor)
{
    Service s(socketPath);
    Client c(socketPath);
}

BOOST_AUTO_TEST_CASE(ServiceAddRemoveMethod)
{
    Service s(socketPath);
    s.addMethodHandler<EmptyData, EmptyData>(1, returnEmptyCallback);
    s.addMethodHandler<SendData, SendData>(1, returnDataCallback);

    s.start();

    s.addMethodHandler<SendData, SendData>(1, echoCallback);
    s.addMethodHandler<SendData, SendData>(2, returnDataCallback);

    Client c(socketPath);
    connect(s, c);
    testEcho(c, 1);

    s.removeMethod(1);
    s.removeMethod(2);

    BOOST_CHECK_THROW(testEcho(c, 2), IPCException);
}

BOOST_AUTO_TEST_CASE(ClientAddRemoveMethod)
{
    Service s(socketPath);
    Client c(socketPath);
    c.addMethodHandler<EmptyData, EmptyData>(1, returnEmptyCallback);
    c.addMethodHandler<SendData, SendData>(1, returnDataCallback);

    FileDescriptor peerFD = connect(s, c);

    c.addMethodHandler<SendData, SendData>(1, echoCallback);
    c.addMethodHandler<SendData, SendData>(2, returnDataCallback);

    testEcho(s, 1, peerFD);

    c.removeMethod(1);
    c.removeMethod(2);

    BOOST_CHECK_THROW(testEcho(s, 1, peerFD), IPCException);
}

BOOST_AUTO_TEST_CASE(ServiceStartStop)
{
    Service s(socketPath);

    s.addMethodHandler<SendData, SendData>(1, returnDataCallback);

    s.start();
    s.stop();
    s.start();
    s.stop();

    s.start();
    s.start();
}

BOOST_AUTO_TEST_CASE(ClientStartStop)
{
    Service s(socketPath);
    Client c(socketPath);
    c.addMethodHandler<SendData, SendData>(1, returnDataCallback);

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
    Service s(socketPath);
    s.addMethodHandler<SendData, SendData>(1, echoCallback);
    s.addMethodHandler<SendData, SendData>(2, echoCallback);

    Client c(socketPath);
    connect(s, c);

    testEcho(c, 1);
    testEcho(c, 2);
}

BOOST_AUTO_TEST_CASE(Restart)
{
    Service s(socketPath);
    s.addMethodHandler<SendData, SendData>(1, echoCallback);
    s.start();
    s.addMethodHandler<SendData, SendData>(2, echoCallback);

    Client c(socketPath);
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
    Service s(socketPath);
    Client c(socketPath);
    c.addMethodHandler<SendData, SendData>(1, echoCallback);
    FileDescriptor peerFD = connect(s, c);

    std::shared_ptr<SendData> sentData(new SendData(56));
    std::shared_ptr<SendData> recvData = s.callSync<SendData, SendData>(1, peerFD, sentData);
    BOOST_CHECK_EQUAL(recvData->intVal, sentData->intVal);
}

BOOST_AUTO_TEST_CASE(AsyncClientToServiceEcho)
{
    // Setup Service and Client
    Service s(socketPath);
    s.addMethodHandler<SendData, SendData>(1, echoCallback);
    s.start();
    Client c(socketPath);
    c.start();

    std::mutex mutex;
    std::condition_variable cv;

    //Async call
    std::shared_ptr<SendData> sentData(new SendData(34));
    std::shared_ptr<SendData> recvData;
    auto dataBack = [&cv, &recvData, &mutex](ipc::Status status, std::shared_ptr<SendData>& data) {
        BOOST_CHECK(status == ipc::Status::OK);
        std::unique_lock<std::mutex> lock(mutex);
        recvData = data;
        cv.notify_one();
    };
    c.callAsync<SendData, SendData>(1, sentData, dataBack);

    // Wait for the response
    std::unique_lock<std::mutex> lock(mutex);
    BOOST_CHECK(cv.wait_for(lock, std::chrono::milliseconds(TIMEOUT), [&recvData]() {
        return static_cast<bool>(recvData);
    }));

    BOOST_CHECK_EQUAL(recvData->intVal, sentData->intVal);
}

BOOST_AUTO_TEST_CASE(AsyncServiceToClientEcho)
{
    Service s(socketPath);
    Client c(socketPath);
    c.addMethodHandler<SendData, SendData>(1, echoCallback);
    FileDescriptor peerFD = connect(s, c);

    // Async call
    std::shared_ptr<SendData> sentData(new SendData(56));
    std::shared_ptr<SendData> recvData;

    std::mutex mutex;
    std::condition_variable cv;
    auto dataBack = [&cv, &recvData, &mutex](ipc::Status status, std::shared_ptr<SendData>& data) {
        BOOST_CHECK(status == ipc::Status::OK);
        std::unique_lock<std::mutex> lock(mutex);
        recvData = data;
        cv.notify_one();
    };

    s.callAsync<SendData, SendData>(1, peerFD, sentData, dataBack);

    // Wait for the response
    std::unique_lock<std::mutex> lock(mutex);
    BOOST_CHECK(cv.wait_for(lock, std::chrono::milliseconds(TIMEOUT), [&recvData]() {
        return recvData.get() != nullptr;
    }));

    BOOST_CHECK_EQUAL(recvData->intVal, sentData->intVal);
}


BOOST_AUTO_TEST_CASE(SyncTimeout)
{
    Service s(socketPath);
    s.addMethodHandler<SendData, SendData>(1, longEchoCallback);

    Client c(socketPath);
    connect(s, c);

    std::shared_ptr<SendData> sentData(new SendData(78));
    BOOST_REQUIRE_THROW((c.callSync<SendData, SendData>(1, sentData, TIMEOUT)), IPCException);
}

BOOST_AUTO_TEST_CASE(SerializationError)
{
    Service s(socketPath);
    s.addMethodHandler<SendData, SendData>(1, echoCallback);

    Client c(socketPath);
    connect(s, c);

    std::shared_ptr<ThrowOnAcceptData> throwingData(new ThrowOnAcceptData());

    BOOST_CHECK_THROW((c.callSync<ThrowOnAcceptData, SendData>(1, throwingData)), IPCSerializationException);

}

BOOST_AUTO_TEST_CASE(ParseError)
{
    Service s(socketPath);
    s.addMethodHandler<SendData, SendData>(1, echoCallback);
    s.start();

    Client c(socketPath);
    c.start();

    std::shared_ptr<SendData> sentData(new SendData(78));
    BOOST_CHECK_THROW((c.callSync<SendData, ThrowOnAcceptData>(1, sentData, 10000)), IPCParsingException);
}

BOOST_AUTO_TEST_CASE(DisconnectedPeerError)
{
    Service s(socketPath);

    auto method = [](const FileDescriptor, std::shared_ptr<ThrowOnAcceptData>&) {
        return std::shared_ptr<SendData>(new SendData(1));
    };

    // Method will throw during serialization and disconnect automatically
    s.addMethodHandler<SendData, ThrowOnAcceptData>(1, method);
    s.start();

    Client c(socketPath);
    c.start();

    std::mutex mutex;
    std::condition_variable cv;
    ipc::Status retStatus = ipc::Status::UNDEFINED;

    auto dataBack = [&cv, &retStatus, &mutex](ipc::Status status, std::shared_ptr<SendData>&) {
        std::unique_lock<std::mutex> lock(mutex);
        retStatus = status;
        cv.notify_one();
    };

    std::shared_ptr<SendData> sentData(new SendData(78));
    c.callAsync<SendData, SendData>(1, sentData, dataBack);

    // Wait for the response
    std::unique_lock<std::mutex> lock(mutex);
    BOOST_CHECK(cv.wait_for(lock, std::chrono::milliseconds(TIMEOUT), [&retStatus]() {
        return retStatus != ipc::Status::UNDEFINED;
    }));

    // The disconnection might have happened:
    // - after sending the message (PEER_DISCONNECTED)
    // - during external serialization (SERIALIZATION_ERROR)
    BOOST_CHECK(retStatus == ipc::Status::PEER_DISCONNECTED || retStatus == ipc::Status::SERIALIZATION_ERROR);
}


BOOST_AUTO_TEST_CASE(ReadTimeout)
{
    Service s(socketPath);
    auto longEchoCallback = [](const FileDescriptor, std::shared_ptr<SendData>& data) {
        return std::shared_ptr<LongSendData>(new LongSendData(data->intVal, LONG_OPERATION_TIME));
    };
    s.addMethodHandler<LongSendData, SendData>(1, longEchoCallback);

    Client c(socketPath);
    connect(s, c);

    // Test timeout on read
    std::shared_ptr<SendData> sentData(new SendData(334));
    BOOST_CHECK_THROW((c.callSync<SendData, SendData>(1, sentData, TIMEOUT)), IPCException);
}


BOOST_AUTO_TEST_CASE(WriteTimeout)
{
    Service s(socketPath);
    s.addMethodHandler<SendData, SendData>(1, echoCallback);
    s.start();

    Client c(socketPath);
    c.start();

    // Test echo with a minimal timeout
    std::shared_ptr<LongSendData> sentDataA(new LongSendData(34, SHORT_OPERATION_TIME));
    std::shared_ptr<SendData> recvData = c.callSync<LongSendData, SendData>(1, sentDataA, TIMEOUT);
    BOOST_CHECK_EQUAL(recvData->intVal, sentDataA->intVal);

    // Test timeout on write
    std::shared_ptr<LongSendData> sentDataB(new LongSendData(34, LONG_OPERATION_TIME));
    BOOST_CHECK_THROW((c.callSync<LongSendData, SendData>(1, sentDataB, TIMEOUT)), IPCTimeoutException);
}


BOOST_AUTO_TEST_CASE(AddSignalInRuntime)
{
    Service s(socketPath);
    Client c(socketPath);
    connect(s, c);

    utils::Latch latchA;
    auto handlerA = [&latchA](const FileDescriptor, std::shared_ptr<SendData>&) {
        latchA.set();
    };

    utils::Latch latchB;
    auto handlerB = [&latchB](const FileDescriptor, std::shared_ptr<SendData>&) {
        latchB.set();
    };

    c.addSignalHandler<SendData>(1, handlerA);
    c.addSignalHandler<SendData>(2, handlerB);

    // Wait for the signals to propagate to the Service
    std::this_thread::sleep_for(std::chrono::milliseconds(2 * TIMEOUT));

    auto data = std::make_shared<SendData>(1);
    s.signal<SendData>(2, data);
    s.signal<SendData>(1, data);

    // Wait for the signals to arrive
    BOOST_CHECK(latchA.wait(TIMEOUT) && latchB.wait(TIMEOUT));
}


BOOST_AUTO_TEST_CASE(AddSignalOffline)
{
    Service s(socketPath);
    Client c(socketPath);

    utils::Latch latchA;
    auto handlerA = [&latchA](const FileDescriptor, std::shared_ptr<SendData>&) {
        latchA.set();
    };

    utils::Latch latchB;
    auto handlerB = [&latchB](const FileDescriptor, std::shared_ptr<SendData>&) {
        latchB.set();
    };

    c.addSignalHandler<SendData>(1, handlerA);
    c.addSignalHandler<SendData>(2, handlerB);

    connect(s, c);

    // Wait for the information about the signals to propagate
    std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT));
    auto data = std::make_shared<SendData>(1);
    s.signal<SendData>(2, data);
    s.signal<SendData>(1, data);

    // Wait for the signals to arrive
    BOOST_CHECK(latchA.wait(TIMEOUT) && latchB.wait(TIMEOUT));
}


#if GLIB_CHECK_VERSION(2,36,0)

BOOST_AUTO_TEST_CASE(ServiceGSource)
{
    ScopedGlibLoop loop;
    utils::Latch l;
    auto signalHandler = [&l](const FileDescriptor, std::shared_ptr<SendData>&) {
        l.set();
    };

    IPCGSource::Pointer serviceGSource;
    Service s(socketPath);
    s.addMethodHandler<SendData, SendData>(1, echoCallback);

    Client c(socketPath);
    s.addSignalHandler<SendData>(2, signalHandler);

    auto ret = connectServiceGSource(s, c);
    serviceGSource = ret.second;

    testEcho(c, 1);

    auto data = std::make_shared<SendData>(1);
    c.signal<SendData>(2, data);

    BOOST_CHECK(l.wait(TIMEOUT));
}

BOOST_AUTO_TEST_CASE(ClientGSource)
{
    ScopedGlibLoop loop;

    utils::Latch l;
    auto signalHandler = [&l](const FileDescriptor, std::shared_ptr<SendData>&) {
        l.set();
    };

    Service s(socketPath);
    s.start();

    IPCGSource::Pointer clientGSource;
    Client c(socketPath);
    c.addMethodHandler<SendData, SendData>(1, echoCallback);
    c.addSignalHandler<SendData>(2, signalHandler);

    auto ret = connectClientGSource(s, c);
    FileDescriptor peerFD = ret.first;
    clientGSource = ret.second;

    testEcho(s, 1, peerFD);

    auto data = std::make_shared<SendData>(1);
    s.signal<SendData>(2, data);

    BOOST_CHECK(l.wait(TIMEOUT));
}

#endif // GLIB_CHECK_VERSION

// BOOST_AUTO_TEST_CASE(ConnectionLimitTest)
// {
//     unsigned oldLimit = ipc::getMaxFDNumber();
//     ipc::setMaxFDNumber(50);

//     // Setup Service and many Clients
//     Service s(socketPath);
//     s.addMethodHandler<SendData, SendData>(1, echoCallback);
//     s.start();

//     std::list<Client> clients;
//     for (int i = 0; i < 100; ++i) {
//         try {
//             clients.push_back(Client(socketPath));
//             clients.back().start();
//         } catch (...) {}
//     }

//     unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
//     std::mt19937 generator(seed);
//     for (auto it = clients.begin(); it != clients.end(); ++it) {
//         try {
//             std::shared_ptr<SendData> sentData(new SendData(generator()));
//             std::shared_ptr<SendData> recvData = it->callSync<SendData, SendData>(1, sentData);
//             BOOST_CHECK_EQUAL(recvData->intVal, sentData->intVal);
//         } catch (...) {}
//     }

//     ipc::setMaxFDNumber(oldLimit);
// }



BOOST_AUTO_TEST_SUITE_END()
