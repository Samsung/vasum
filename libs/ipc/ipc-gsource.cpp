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
 * @brief   Class for creating a dedicated GSource
 */


#include "config.hpp"

#include "ipc/ipc-gsource.hpp"
#include "utils/callback-wrapper.hpp"
#include "logger/logger.hpp"

#include <algorithm>

namespace ipc {

namespace {

gushort conditions = static_cast<gushort>(G_IO_IN |
                                          G_IO_ERR |
                                          G_IO_HUP);

}

IPCGSource::IPCGSource(const HandlerCallback& handlerCallback)
    : mHandlerCallback(handlerCallback)
{
    LOGT("IPCGSource Constructor");
}

IPCGSource::~IPCGSource()
{
    LOGT("IPCGSource Destructor");
}

IPCGSource::Pointer IPCGSource::create(const HandlerCallback& handlerCallback)
{
    LOGT("Creating IPCGSource");

    static GSourceFuncs funcs = { &IPCGSource::prepare,
                                  &IPCGSource::check,
                                  &IPCGSource::dispatch,
                                  &IPCGSource::finalize,
                                  nullptr,
                                  nullptr
                                };

    // New GSource
    GSource* gSource = g_source_new(&funcs, sizeof(IPCGSource));
    g_source_set_priority(gSource, G_PRIORITY_HIGH);

    // Fill additional data
    IPCGSource* source = reinterpret_cast<IPCGSource*>(gSource);
    new(source) IPCGSource(handlerCallback);

    auto deleter = [](IPCGSource * ptr) {
        LOGD("Deleter");
        g_source_unref(&ptr->mGSource);
    };

    Pointer ipcGSourcePtr(source, deleter);

    g_source_set_callback(gSource,
                          &IPCGSource::onHandlerCall,
                          utils::createCallbackWrapper(Pointer(ipcGSourcePtr), ipcGSourcePtr->mGuard.spawn()),
                          &utils::deleteCallbackWrapper<Pointer>);

    return ipcGSourcePtr;
}

void IPCGSource::addFD(const FileDescriptor fd)
{
    LOGI("Adding to glib FD: " << fd);
    Lock lock(mStateMutex);

    mGPollFDs.push_back({fd, conditions, 0});
    g_source_add_poll(&mGSource, &mGPollFDs.back());
}

void IPCGSource::removeFD(const FileDescriptor fd)
{
    Lock lock(mStateMutex);

    auto it = std::find_if(mGPollFDs.begin(), mGPollFDs.end(), [fd](GPollFD gPollFD) {
        return gPollFD.fd = fd;
    });

    if (it == mGPollFDs.end()) {
        LOGE("No such fd");
        return;
    }
    g_source_remove_poll(&mGSource, &(*it));
    mGPollFDs.erase(it);
    LOGI("Removed from glib FD: " << fd);
}

guint IPCGSource::attach(GMainContext* context)
{
    LOGT("Attaching to GMainContext");
    guint ret = g_source_attach(&mGSource, context);
    return ret;
}

void IPCGSource::detach()
{
    LOGT("Detaching");
    Lock lock(mStateMutex);

    for (GPollFD gPollFD : mGPollFDs) {
        g_source_remove_poll(&mGSource, &gPollFD);
    }

    mGPollFDs.clear();
    if (!g_source_is_destroyed(&mGSource)) {
        LOGD("Destroying");
        // This way finalize method will be run in glib loop's thread
        g_source_destroy(&mGSource);
    }
}

void IPCGSource::callHandler()
{
    Lock lock(mStateMutex);

    for (const GPollFD& gPollFD : mGPollFDs) {
        if (gPollFD.revents & conditions) {
            mHandlerCallback(gPollFD.fd, gPollFD.revents);
        }
    }
}

gboolean IPCGSource::onHandlerCall(gpointer userData)
{
    const auto& source = utils::getCallbackFromPointer<Pointer>(userData);
    if (source) {
        source->callHandler();
    }
    return TRUE;
}

gboolean IPCGSource::prepare(GSource* gSource, gint* timeout)
{
    if (!gSource || g_source_is_destroyed(gSource)) {
        return FALSE;
    }

    *timeout = -1;

    // TODO: Implement hasEvents() method in Client and Service and use it here as a callback:
    // return source->hasEvents();
    return FALSE;
}

gboolean IPCGSource::check(GSource* gSource)
{
    if (!gSource || g_source_is_destroyed(gSource)) {
        return FALSE;
    }

    return TRUE;
}

gboolean IPCGSource::dispatch(GSource* gSource,
                              GSourceFunc callback,
                              gpointer userData)
{
    if (!gSource || g_source_is_destroyed(gSource)) {
        // Remove the GSource from the GMainContext
        return FALSE;
    }

    if (callback) {
        callback(userData);
    }

    return TRUE;
}

void  IPCGSource::finalize(GSource* gSource)
{
    if (gSource) {
        IPCGSource* source = reinterpret_cast<IPCGSource*>(gSource);
        source->~IPCGSource();
    }
}

} // namespace ipc
