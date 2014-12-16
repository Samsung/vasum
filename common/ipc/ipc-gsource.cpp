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
 * @brief   Class for creating a dedicated GSource
 */


#include "config.hpp"

#include "ipc/ipc-gsource.hpp"

#if GLIB_CHECK_VERSION(2,36,0)

#include "logger/logger.hpp"
#include <algorithm>

namespace vasum {
namespace ipc {

namespace {


GIOCondition conditions = static_cast<GIOCondition>(G_IO_IN |
                                                    G_IO_ERR |
                                                    G_IO_HUP);
}


IPCGSource::IPCGSource(const std::vector<FileDescriptor> fds,
                       const HandlerCallback& handlerCallback)
    : mHandlerCallback(handlerCallback)
{
    LOGD("Constructing IPCGSource");
    for (const FileDescriptor fd : fds) {
        addFD(fd);
    }
}

IPCGSource::~IPCGSource()
{
    LOGD("Destroying IPCGSource");
}

IPCGSource::Pointer IPCGSource::create(const std::vector<FileDescriptor>& fds,
                                       const HandlerCallback& handlerCallback)
{
    LOGD("Creating IPCGSource");

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
    new(source)IPCGSource(fds, handlerCallback);

    auto deleter = [](IPCGSource * ptr) {
        LOGD("Deleter");

        if (!g_source_is_destroyed(&(ptr->mGSource))) {
            // This way finalize method will be run in glib loop's thread
            g_source_destroy(&(ptr->mGSource));
        }
    };

    return std::shared_ptr<IPCGSource>(source, deleter);
}

void IPCGSource::addFD(const FileDescriptor fd)
{
    if (!&mGSource) {
        // In case it's called as a callback but the IPCGSource is destroyed
        return;
    }

    LOGD("Adding fd to glib");
    gpointer tag = g_source_add_unix_fd(&mGSource,
                                        fd,
                                        conditions);
    FDInfo fdInfo(tag, fd);
    mFDInfos.push_back(std::move(fdInfo));
}

void IPCGSource::removeFD(const FileDescriptor fd)
{
    if (!&mGSource) {
        // In case it's called as a callback but the IPCGSource is destroyed
        return;
    }

    LOGD("Removing fd from glib");
    auto it = std::find(mFDInfos.begin(), mFDInfos.end(), fd);
    if (it == mFDInfos.end()) {
        LOGE("No such fd");
        return;
    }
    g_source_remove_unix_fd(&mGSource, it->tag);
    mFDInfos.erase(it);
}

guint IPCGSource::attach(GMainContext* context)
{
    LOGD("Attaching to GMainContext");
    guint ret = g_source_attach(&mGSource, context);
    g_source_unref(&mGSource);
    return ret;
}

gboolean IPCGSource::prepare(GSource* gSource, gint* timeout)
{
    if (!gSource) {
        return FALSE;
    }

    *timeout = -1;

    // TODO: Implement hasEvents() method in Client and Service and use it here as a callback:
    // return source->hasEvents();
    return FALSE;
}

gboolean IPCGSource::check(GSource* gSource)
{
    if (!gSource) {
        return FALSE;
    }

    return TRUE;
}

gboolean IPCGSource::dispatch(GSource* gSource,
                              GSourceFunc /*callback*/,
                              gpointer /*userData*/)
{
    if (!gSource || g_source_is_destroyed(gSource)) {
        // Remove the GSource from the GMainContext
        return FALSE;
    }

    IPCGSource* source = reinterpret_cast<IPCGSource*>(gSource);

    for (const FDInfo fdInfo : source->mFDInfos) {
        GIOCondition cond = g_source_query_unix_fd(gSource, fdInfo.tag);
        if (conditions & cond) {
            source->mHandlerCallback(fdInfo.fd, cond);
        }
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
} // namespace vasum

#endif // GLIB_CHECK_VERSION
