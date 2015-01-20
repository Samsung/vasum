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
 * @brief    Class for creating a dedicated GSource
 */

#ifndef COMMON_IPC_IPC_GSOURCE_HPP
#define COMMON_IPC_IPC_GSOURCE_HPP

#include <glib.h>
#include "ipc/types.hpp"
#include "utils/callback-guard.hpp"

#include <memory>
#include <mutex>
#include <list>


namespace vasum {
namespace ipc {

/**
 * Class for connecting to the glib's loop.
 * Creates a dedicated GSource.
 *
 * It's supposed to be constructed ONLY with the static create method
 * and destructed in a glib callback.
 *
 * TODO:
 * - waiting till the managed object (Client or Service) is destroyed
 *   before IPCGSource stops operating. For now programmer has to ensure this.
 */
struct IPCGSource {
public:
    typedef std::function<void(FileDescriptor fd, short pollEvent)> HandlerCallback;
    typedef std::shared_ptr<IPCGSource> Pointer;

    ~IPCGSource();

    IPCGSource() = delete;
    IPCGSource(const IPCGSource&) = delete;
    IPCGSource& operator=(const IPCGSource&) = delete;

    /**
     * New file descriptor to listen on.
     *
     * @param peerFD file descriptor
     */
    void addFD(const FileDescriptor peerFD);

    /**
     * Removes the file descriptor from the GSource
     *
     * @param peerFD file descriptor
     */
    void removeFD(const FileDescriptor peerFD);

    /**
     * Attach to the glib's GMainContext
     *
     * @param context where to connect
     * @return result of the g_source_attach call
     */
    guint attach(GMainContext* context = nullptr);

    /**
     * After this method quits handlerCallback will not be called
     */
    void detach();

    /**
     * Creates the IPCGSource class in the memory allocated by glib.
     * Calls IPCGSource's constructor
     *
     * @param handlerCallback event handling callback
     *
     * @return pointer to the IPCGSource
     */
    static Pointer create(const HandlerCallback& handlerCallback);

    /**
     * Callback for the dispatch function
     */
    static gboolean onHandlerCall(gpointer userData);

    /**
     * Locks the internal state mutex and calls the handler callback for each fd
     */
    void callHandler();

private:
    typedef std::unique_lock<std::recursive_mutex> Lock;

    /**
     * GSourceFuncs' callback
     */
    static gboolean prepare(GSource* source, gint* timeout);

    /**
     * GSourceFuncs' callback
     */
    static gboolean check(GSource* source);

    /**
     * GSourceFuncs' callback
     */
    static gboolean dispatch(GSource* source,
                             GSourceFunc callbacks,
                             gpointer userData);

    /**
     * GSourceFuncs' callback
     */
    static void  finalize(GSource* source);

    // Called only from IPCGSource::create
    IPCGSource(const HandlerCallback& handlerCallback);

    GSource mGSource;
    HandlerCallback mHandlerCallback;
    std::list<GPollFD> mGPollFDs;
    utils::CallbackGuard mGuard;
    std::recursive_mutex mStateMutex;

};

} // namespace ipc
} // namespace vasum

#endif // COMMON_IPC_IPC_GSOURCE_HPP
