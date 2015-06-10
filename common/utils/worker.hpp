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

#ifndef COMMON_UTILS_WORKER_HPP
#define COMMON_UTILS_WORKER_HPP

#include <functional>
#include <memory>

namespace utils {

/**
 * A queue with tasks executed in a dedicated thread.
 * Current implementation creates a thread on the first use.
 */
class Worker {
public:
    typedef std::shared_ptr<Worker> Pointer;
    typedef std::function<void()> Task;

    ~Worker();

    /**
     * Creates a worker with its own thread
     */
    static Pointer create();

    /**
     * Creates a worker that share a thread with its parent
     */
    Pointer createSubWorker();

    /**
     * Adds a task to the queue.
     */
    void addTask(const Task& task);
    void addTaskAndWait(const Task& task);

private:
    typedef unsigned int GroupID;
    class WorkerQueue;

    const std::shared_ptr<WorkerQueue> mWorkerQueue;
    const GroupID mGroupID;

    Worker(const std::shared_ptr<WorkerQueue>& workerQueue);
};

} // namespace utils


#endif // COMMON_UTILS_WORKER_HPP
