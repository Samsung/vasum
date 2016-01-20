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

#include "config.hpp"

#include "utils/inotify.hpp"
#include "utils/paths.hpp"
#include "utils/fs.hpp"
#include "utils/fd-utils.hpp"
#include "utils/exception.hpp"

#include "logger/logger.hpp"
#include "logger/logger-scope.hpp"

#include <sys/ioctl.h>

#include <functional>


namespace utils {

Inotify::Inotify(cargo::ipc::epoll::EventPoll& eventPoll)
    :mEventPoll(eventPoll)
{
    mFD = ::inotify_init1(IN_CLOEXEC);
    if (mFD == -1) {
        const std::string msg = "Error in inotify_init1: " + getSystemErrorMessage();
        LOGE(msg);
        throw UtilsException(msg);
    }

    mEventPoll.addFD(mFD, EPOLLIN, std::bind(&Inotify::handleInternal, this));
}

Inotify::~Inotify()
{
    LOGS("~Inotify");
    {
        Lock lock(mMutex);

        for(const auto& handler: mHandlers) {
            if (-1 == ::inotify_rm_watch(mFD, handler.watchID)) {
                LOGE("Error in inotify_rm_watch: " + getSystemErrorMessage());
            }
        }
    }

    mEventPoll.removeFD(mFD);
    utils::close(mFD);
}

int Inotify::getFD() const
{
    return mFD;
}

void Inotify::setHandler(const std::string& path,
                         const uint32_t eventMask,
                         const Callback&& callback)
{
    LOGT("Added inotify for: " << path);
    Lock lock(mMutex);

    removeHandlerInternal(path);

    int watchID = ::inotify_add_watch(mFD, path.c_str(), eventMask);
    if (-1 == watchID) {
        const std::string msg = "Error in inotify_add_watch: " + getSystemErrorMessage();
        LOGE(msg);
        throw UtilsException(msg);
    }

    mHandlers.push_back({path, watchID, callback});
}

void Inotify::removeHandlerInternal(const std::string& path)
{
    // Find the corresponding handler's data
    auto it = std::find_if(mHandlers.begin(), mHandlers.end(), [&path](const Handler& h) {
        return path == h.path;
    });

    if (it == mHandlers.end()) {
        return;
    }

    // Unwatch the path
    if (-1 == ::inotify_rm_watch(mFD, it->watchID)) {
        const std::string msg = "Error in inotify_rm_watch: " + getSystemErrorMessage();
        LOGE(msg);
        throw UtilsException(msg);
    }

    mHandlers.erase(it);
}

void Inotify::removeHandler(const std::string& path)
{
    LOGT("Removed inotify for: " << path);
    Lock lock(mMutex);
    removeHandlerInternal(path);
}

void Inotify::handleInternal()
{
    Lock lock(mMutex);

    // Get how much data is awaiting
    unsigned int bufferSize;
    utils::ioctl(mFD, FIONREAD, &bufferSize);

    // Read all events into a buffer
    std::vector<char> buffer(bufferSize);
    utils::read(mFD, buffer.data(), bufferSize);

    // Handle all events
    unsigned int offset = 0;
    while (offset < bufferSize) {
        struct ::inotify_event *event = reinterpret_cast<struct ::inotify_event*>(&buffer[offset]);
        offset = offset + sizeof(inotify_event) + event->len;

        if(event->mask & IN_IGNORED) {
            // Watch was removed - ignore
            continue;
        }

        auto it = std::find_if(mHandlers.begin(), mHandlers.end(), [event](const Handler& h) {
            return event->wd == h.watchID;
        });
        if (it == mHandlers.end()) {
            // Meantime the callback was deleted by another callback
            LOGE("No callback for file: " << event->name);
            continue;
        }

        LOGT("Handling inotify: " << event->name);
        it->call(event->name, event->mask);
    }
}

} // namespace utils
