/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Piotr Bartosiewicz <p.bartosiewi@partner.samsung.com>
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
 * @author  Piotr Bartosiewicz (p.bartosiewi@partner.samsung.com)
 * @brief   Unit tests of worker thread
 */

#include "config.hpp"

#include "ut.hpp"

#include "utils/worker.hpp"
#include "utils/latch.hpp"

#include <chrono>
#include <thread>
#include <atomic>

using namespace utils;

BOOST_AUTO_TEST_SUITE(WorkerSuite)

const int unsigned TIMEOUT = 1000;

BOOST_AUTO_TEST_CASE(NoTasks)
{
    Worker::Pointer worker = Worker::create();
}

BOOST_AUTO_TEST_CASE(NoTasksWithSubWorkers)
{
    Worker::Pointer worker = Worker::create();
    Worker::Pointer sub1 = worker->createSubWorker();
    Worker::Pointer sub2 = worker->createSubWorker();
    Worker::Pointer sub3 = sub1->createSubWorker();

    sub1.reset();
    worker.reset();
}

BOOST_AUTO_TEST_CASE(Simple)
{
    Latch done;

    Worker::Pointer worker = Worker::create();
    worker->addTask([&] {
        done.set();
    });

    BOOST_CHECK(done.wait(TIMEOUT));
}

BOOST_AUTO_TEST_CASE(Queue)
{
    std::mutex mutex;
    std::string result;

    Worker::Pointer worker = Worker::create();

    for (int n=0; n<10; ++n) {
        worker->addTask([&, n]{
            std::lock_guard<std::mutex> lock(mutex);
            result += std::to_string(n);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        });
    }

    worker.reset();

    std::lock_guard<std::mutex> lock(mutex);
    BOOST_CHECK_EQUAL("0123456789", result);
}

BOOST_AUTO_TEST_CASE(ThreadResume)
{
    Latch done;

    const auto task = [&] {
        done.set();
    };

    Worker::Pointer worker = Worker::create();

    worker->addTask(task);

    BOOST_CHECK(done.wait(TIMEOUT));

    // make sure worker thread is in waiting state
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    worker->addTask(task);

    worker.reset();

    BOOST_CHECK(done.wait(TIMEOUT));
}

BOOST_AUTO_TEST_CASE(SubWorker)
{
    std::mutex mutex;
    std::string result;

    Worker::Pointer worker = Worker::create();
    Worker::Pointer sub1 = worker->createSubWorker();
    Worker::Pointer sub2 = worker->createSubWorker();

    auto addTask = [&](Worker::Pointer w, const std::string& id) {
        w->addTask([&, id]{
            std::lock_guard<std::mutex> lock(mutex);
            result += id;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        });
    };

    for (int n=0; n<4; ++n) {
        addTask(worker, "_w" + std::to_string(n));
        addTask(sub1, "_a" + std::to_string(n));
    }

    worker.reset();
    sub1.reset();

    {
        std::lock_guard<std::mutex> lock(mutex);
        BOOST_CHECK_EQUAL("_w0_a0_w1_a1_w2_a2_w3_a3", result);
        result.clear();
    }

    addTask(sub2, "_b0");
    addTask(sub2, "_b1");

    sub2.reset();

    {
        std::lock_guard<std::mutex> lock(mutex);
        BOOST_CHECK_EQUAL("_b0_b1", result);
    }
}

BOOST_AUTO_TEST_CASE(NoCopy)
{
    typedef std::atomic_int Counter;

    struct Task {
        Counter& count;

        Task(Counter& c) : count(c) {}
        Task(const Task& t) : count(t.count) {++count;}
        Task(Task&& r) : count(r.count) {}
        Task& operator=(const Task&) = delete;
        Task& operator=(Task&&) = delete;
        void operator() () const {}

    };

    Counter copyCount(0);

    Worker::Pointer worker = Worker::create();
    worker->addTask(Task(copyCount));
    worker.reset();

    BOOST_CHECK_EQUAL(1, copyCount); // one copy for creating std::function
}

BOOST_AUTO_TEST_SUITE_END()
