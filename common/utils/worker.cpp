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
 * @brief   A worker thread that executes tasks
 */

#include "config.hpp"

#include "utils/worker.hpp"
#include "utils/latch.hpp"
#include "utils/counting-map.hpp"
#include "logger/logger.hpp"

#include <atomic>
#include <deque>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <cassert>


namespace utils {


class Worker::WorkerQueue {
public:
    WorkerQueue()
        : mLastGroupID(0), mEnding(false)
    {
        LOGT("Worker queue created");
    }

    ~WorkerQueue()
    {
        {
            Lock lock(mMutex);
            assert(mTaskQueue.empty());
            assert(mGroupCounter.empty());
            mEnding = true;
        }
        if (mThread.joinable()) {
            mAddedCondition.notify_all();
            mThread.join();
        }
        LOGT("Worker queue destroyed");
    }

    GroupID getNextGroupID()
    {
        return ++mLastGroupID;
    }

    void addTask(const Worker::Task& task, GroupID groupID, Latch* latch)
    {
        assert(task);

        Lock lock(mMutex);
        LOGT("Adding task to subgroup " << groupID);
        mTaskQueue.push_back(TaskInfo{task, groupID, latch});
        mGroupCounter.increment(groupID);
        mAddedCondition.notify_one();
        if (!mThread.joinable()) {
            mThread = std::thread(&WorkerQueue::workerProc, this);
        }
    }

    void waitForGroupEmpty(GroupID groupID)
    {
        Lock lock(mMutex);
        size_t count = mGroupCounter.get(groupID);
        if (count > 0) {
            LOGD("Waiting for " << count << " task in group " << groupID);
        }
        mEmptyGroupCondition.wait(lock, [this, groupID] {
            return mGroupCounter.get(groupID) == 0;
        });
    }
private:
    typedef std::unique_lock<std::mutex> Lock;

    struct TaskInfo {
        Worker::Task task;
        GroupID groupID;
        Latch *latch;
    };

    std::atomic<GroupID> mLastGroupID;
    std::condition_variable mAddedCondition;
    std::condition_variable mEmptyGroupCondition;
    std::thread mThread;

    std::mutex mMutex; // protects below member variables:
    bool mEnding;
    std::deque<TaskInfo> mTaskQueue;
    CountingMap<GroupID> mGroupCounter;

    void workerProc()
    {
        LOGT("Worker thread started");
        for (;;) {
            // wait for a task
            GroupID groupID;
            {
                Lock lock(mMutex);
                mAddedCondition.wait(lock, [this] {
                    return !mTaskQueue.empty() || mEnding;
                });
                if (mTaskQueue.empty()) {
                    break;
                }
                TaskInfo taskInfo = std::move(mTaskQueue.front());
                mTaskQueue.pop_front();

                lock.unlock();

                // execute
                execute(taskInfo);
                groupID = taskInfo.groupID;
            }
            // remove from queue
            {
                Lock lock(mMutex);
                if (mGroupCounter.decrement(groupID) == 0) {
                    mEmptyGroupCondition.notify_all();
                }
            }
        }
        LOGT("Worker thread exited");
    }

    static void execute(const TaskInfo& taskInfo)
    {
        try {
            LOGT("Executing task from subgroup " << taskInfo.groupID);
            taskInfo.task();
        } catch (const std::exception& e) {
            LOGE("Unexpected exception while executing task: " << e.what());
        }
        if (taskInfo.latch)
            taskInfo.latch->set();
    }
};


Worker::Pointer Worker::create()
{
    return Pointer(new Worker(std::make_shared<WorkerQueue>()));
}

Worker::Worker(const std::shared_ptr<WorkerQueue>& workerQueue)
    : mWorkerQueue(workerQueue), mGroupID(workerQueue->getNextGroupID())
{
}

Worker::~Worker()
{
    mWorkerQueue->waitForGroupEmpty(mGroupID);
}

Worker::Pointer Worker::createSubWorker()
{
    return Pointer(new Worker(mWorkerQueue));
}

void Worker::addTask(const Task& task)
{
    mWorkerQueue->addTask(task, mGroupID, NULL);
}

void Worker::addTaskAndWait(const Task& task)
{
    Latch latch;

    mWorkerQueue->addTask(task, mGroupID, &latch);
    latch.wait();
}

} // namespace utils
