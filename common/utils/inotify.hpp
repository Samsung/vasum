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
 * @brief   Inotify wrapper
 */

#ifndef COMMON_UTILS_INOTIFY_HPP
#define COMMON_UTILS_INOTIFY_HPP

#include "cargo-ipc/epoll/event-poll.hpp"

#include <functional>
#include <mutex>
#include <vector>

#include <sys/inotify.h>


namespace utils {

/**
 * Inotify monitors a directory and when a specified file or folder
 * is created or deleted it calls a corresponding handler.
 */
class Inotify {
public:
    typedef std::function<void(const std::string&, const uint32_t)> Callback;

    Inotify(cargo::ipc::epoll::EventPoll& eventPoll);
    virtual ~Inotify();

    Inotify(const Inotify&) = delete;
    Inotify& operator=(const Inotify&) = delete;

    /**
     * Add a callback for a specified path
     */
    void setHandler(const std::string& path, const uint32_t eventMask, const Callback&& callback);

    /**
     * Stop watching the path
     */
    void removeHandler(const std::string& path);

    /**
     * @return inotify file descriptor
     */
    int getFD() const;

private:
    struct Handler {
        std::string path;
        int watchID;
        Callback call;
    };

    typedef std::lock_guard<std::recursive_mutex> Lock;
    std::recursive_mutex mMutex;

    int mFD;
    cargo::ipc::epoll::EventPoll& mEventPoll;
    std::vector<Handler> mHandlers;

    void handleInternal();
    void removeHandlerInternal(const std::string& path);
};

} // namespace utils

#endif // COMMON_UTILS_INOTIFY_HPP
