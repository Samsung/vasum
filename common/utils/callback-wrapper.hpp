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
 * @brief   Callback wrapper
 */

#ifndef COMMON_UTILS_CALLBACK_WRAPPER_HPP
#define COMMON_UTILS_CALLBACK_WRAPPER_HPP

#include "callback-guard.hpp"


namespace security_containers {
namespace utils {


/**
 * Wraps callback and callback tracker into single object
 */
template<class Callback>
class CallbackWrapper {
public:
    CallbackWrapper(const Callback& callback, const CallbackGuard::Tracker& tracker)
        : mCallback(callback)
        , mTracker(tracker)
    {
    }

    /**
     * @return Wrapped callback
     */
    const Callback& get() const
    {
        return mCallback;
    }
private:
    Callback mCallback;
    CallbackGuard::Tracker mTracker;
};

/**
 * Creates callback wrapper. Useful for C callback api.
 */
template<class Callback>
CallbackWrapper<Callback>* createCallbackWrapper(const Callback& callback,
                                                 const CallbackGuard::Tracker& tracker)
{
    return new CallbackWrapper<Callback>(callback, tracker);
}

/**
 * Deletes callback wrapper. Useful for C callback api.
 */
template<class Callback>
void deleteCallbackWrapper(void* pointer)
{
    delete reinterpret_cast<CallbackWrapper<Callback>*>(pointer);
}

/**
 * Recovers callback from wrapper pointer. Useful for C callback api.
 */
template<class Callback>
const Callback& getCallbackFromPointer(const void* pointer)
{
    return reinterpret_cast<const CallbackWrapper<Callback>*>(pointer)->get();
}


} // namespace utils
} // namespace security_containers


#endif // COMMON_UTILS_CALLBACK_WRAPPER_HPP
