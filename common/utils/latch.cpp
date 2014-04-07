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
 * @brief   Synchronization latch
 */

#include "utils/latch.hpp"


namespace security_containers {
namespace utils {


Latch::Latch()
    : mCount(0)
{
}

void Latch::set()
{
    std::unique_lock<std::mutex> lock(mMutex);
    ++mCount;
    mCondition.notify_one();
}

void Latch::wait()
{
    std::unique_lock<std::mutex> lock(mMutex);
    mCondition.wait(lock, [this] {return mCount > 0;});
    --mCount;
}

bool Latch::wait(const unsigned int timeoutMs)
{
    std::unique_lock<std::mutex> lock(mMutex);
    if (!mCondition.wait_for(lock, std::chrono::milliseconds(timeoutMs),
                             [this] {return mCount > 0;})) {
        return false;
    }
    --mCount;
    return true;
}

bool Latch::empty()
{
    std::unique_lock<std::mutex> lock(mMutex);
    return mCount == 0;
}


} // namespace utils
} // namespace security_containers
