/*
*  Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
*
*  Contact: Lukasz Kostyra <l.kostyra@samsung.com>
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
 * @author  Lukasz Kostyra (l.kostyra@samsung.com)
 * @brief   Definition of ValueLatch template, used to wait for variable to be set.
 */

#ifndef COMMON_UTILS_VALUE_LATCH_H
#define COMMON_UTILS_VALUE_LATCH_H

#include "utils/exception.hpp"

#include <memory>
#include <mutex>
#include <condition_variable>

namespace utils {

template <typename T>
class ValueLatch {
public:
    /**
     * Assigns value to kept variable and sets Latch.
     *
     * @param value Value to set.
     */
    void set(const T& value);

    /**
     * Assigns value to kept variable and sets Latch.
     *
     * @param value Value to set.
     */
    void set(T&& value);

    /**
     * Waits until set() is called, then set value is moved to caller.
     *
     * @return Value provided by set().
     */
    T get();

    /**
     * Waits until set() is called, or until timeout occurs. Then, set value is moved to caller.
     *
     * @param timeoutMs Maximum time to wait for value to be set.
     *
     * @return Value provided by set().
     */
    T get(const unsigned int timeoutMs);

private:
    std::mutex mMutex;
    std::condition_variable mCondition;
    std::unique_ptr<T> mValue;
};

template <typename T>
void ValueLatch<T>::set(const T& value)
{
    std::unique_lock<std::mutex> lock(mMutex);
    if (mValue) {
        throw UtilsException("Cannot set value multiple times");
    }
    mValue.reset(new T(value));
    mCondition.notify_one();
}

template <typename T>
void ValueLatch<T>::set(T&& value)
{
    std::unique_lock<std::mutex> lock(mMutex);
    if (mValue) {
        throw UtilsException("Cannot set value multiple times");
    }
    mValue.reset(new T(std::move(value)));
    mCondition.notify_one();
}

template <typename T>
T ValueLatch<T>::get()
{
    std::unique_lock<std::mutex> lock(mMutex);
    mCondition.wait(lock, [this]() {
        return (bool)mValue;
    });
    std::unique_ptr<T> retValue(std::move(mValue));
    return T(std::move(*retValue));
}

template <typename T>
T ValueLatch<T>::get(const unsigned int timeoutMs)
{
    std::unique_lock<std::mutex> lock(mMutex);
    if (mCondition.wait_for(lock, std::chrono::milliseconds(timeoutMs), [this]() {
                                return (bool)mValue;
                            }) ) {
        std::unique_ptr<T> retValue(std::move(mValue));
        return T(std::move(*retValue));
    } else {
        throw UtilsException("Timeout occured");
    }
}

} // namespace utils

#endif // COMMON_UTILS_VALUE_LATCH_H
