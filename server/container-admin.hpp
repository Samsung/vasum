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
 * @brief   Declaration of the class for administrating one container
 */


#ifndef SERVER_CONTAINER_ADMIN_HPP
#define SERVER_CONTAINER_ADMIN_HPP

#include "container-config.hpp"

#include "utils/callback-guard.hpp"
#include "utils/callback-wrapper.hpp"
#include "libvirt/connection.hpp"
#include "libvirt/domain.hpp"

#include <map>
#include <mutex>
#include <atomic>
#include <string>
#include <cstdint>
#include <libvirt/libvirt.h>


namespace security_containers {


enum class SchedulerLevel {
    FOREGROUND,
    BACKGROUND
};

class ContainerAdmin {

public:
    /**
     * A listener ID type.
     */
    typedef unsigned int ListenerId;

    /**
     * A function type used for the lifecycle listener
     */
    typedef std::function<void(const int eventId, const int detailId)> LifecycleListener;

    /**
     * A function type used for the reboot listener
     */
    typedef std::function<void()> RebootListener;

    ContainerAdmin(ContainerConfig& config);
    virtual ~ContainerAdmin();

    /**
     * Get the container id
     */
    const std::string& getId() const;

    /**
     * Boot the container to the background.
     */
    void start();

    /**
     * Try to shutdown the container, if failed, destroy it.
     */
    void stop();

    /**
     * Forcefully stop the container.
     */
    void destroy();

    /**
     * Gracefully shutdown the container.
     * This method will NOT block until container is shut down.
     */
    void shutdown();

    /**
     * @return Is the container running?
     */
    bool isRunning();

    /**
     * Check if the container is stopped. It's NOT equivalent to !isRunning,
     * because it checks different internal libvirt's states. There are other states,
     * (e.g. paused) when the container isn't running nor stopped.
     *
     * @return Is the container stopped?
     */
    bool isStopped();

    /**
     * Suspends an active container, the process is frozen
     * without further access to CPU resources and I/O,
     * but the memory used by the container
     * at the hypervisor level will stay allocated
     */
    void suspend();

    /**
     * Resume the container after suspension.
     */
    void resume();

    /**
     * @return Is the container in a paused state?
     */
    bool isPaused();

    /**
     * Sets the containers scheduler CFS quota.
     */
    void setSchedulerLevel(SchedulerLevel sched);

    /**
     * Set whether container should be detached on exit.
     */
    void setDetachOnExit();

    /**
     * @return Scheduler CFS quota,
     * TODO: this function is only for UNIT TESTS
     */
    std::int64_t getSchedulerQuota();

    /**
     * Sets a listener for a specific event.
     * It's a caller's responsibility to remove the listener
     * prior to destroying the object.
     *
     * @return listener ID that can be used to remove.
     */
    template <typename Listener>
    ListenerId registerListener(const Listener& listener,
                                const utils::CallbackGuard::Tracker& tracker);

    /**
     * Remove a previously registered listener.
     */
    template <typename Listener>
    void removeListener(const ListenerId id);

private:
    ContainerConfig& mConfig;
    libvirt::LibvirtDomain mDom;
    const std::string mId;
    bool mDetachOnExit;

    int getState();   // get the libvirt's domain state
    void setSchedulerParams(std::uint64_t cpuShares, std::uint64_t vcpuPeriod, std::int64_t vcpuQuota);

    // for handling libvirt callbacks
    utils::CallbackGuard mLibvirtGuard;
    int mLifecycleCallbackId;
    int mRebootCallbackId;

    // virConnectDomainEventCallback
    static int libvirtLifecycleCallback(virConnectPtr con,
                                        virDomainPtr dom,
                                        int event,
                                        int detail,
                                        void* opaque);

    // virConnectDomainEventGenericCallback
    static void libvirtRebootCallback(virConnectPtr con,
                                      virDomainPtr dom,
                                      void* opaque);

    // for handling external listeners triggered from libvirt callbacks
    // TODO, the Listener type might not be unique, reimplement using proper listeners
    template <typename Listener>
    using ListenerMap = std::map<ListenerId, utils::CallbackWrapper<Listener>>;

    std::mutex mListenerMutex;
    std::atomic<unsigned int> mNextIdForListener;
    ListenerMap<LifecycleListener> mLifecycleListeners;
    ListenerMap<RebootListener> mRebootListeners;

    template <typename Listener>
    ListenerMap<Listener>& getListenerMap();
};

template <typename Listener>
unsigned int ContainerAdmin::registerListener(const Listener& listener,
                                              const utils::CallbackGuard::Tracker& tracker)
{

    ListenerMap<Listener>& map = getListenerMap<Listener>();
    unsigned int id = mNextIdForListener++;
    utils::CallbackWrapper<Listener> wrap(listener, tracker);

    std::unique_lock<std::mutex> lock(mListenerMutex);
    map.emplace(id, std::move(wrap));

    return id;
}

template <typename Listener>
void ContainerAdmin::removeListener(const ContainerAdmin::ListenerId id)
{
    ListenerMap<Listener>& map = getListenerMap<Listener>();

    std::unique_lock<std::mutex> lock(mListenerMutex);
    map.erase(id);
}


} // namespace security_containers


#endif // SERVER_CONTAINER_ADMIN_HPP
