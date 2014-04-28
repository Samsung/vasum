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
 * @brief   Implementation of class for administrating one container
 */

#include "container-admin.hpp"
#include "exception.hpp"

#include "libvirt/helpers.hpp"
#include "log/logger.hpp"
#include "utils/fs.hpp"
#include "utils/latch.hpp"
#include "utils/callback-wrapper.hpp"

#include <cassert>
#include <cstring>
#include <string>
#include <memory>
#include <cstdint>


namespace security_containers {


namespace {

// TODO: this should be in container's configuration file
const int SHUTDOWN_WAIT = 10 * 1000;

std::string getDomainName(virDomainPtr dom)
{
    assert(dom != NULL);

    const char* name;
    if ((name = virDomainGetName(dom)) == NULL) {
        LOGE("Failed to get the domain's id:\n"
             << libvirt::libvirtFormatError());
        throw ContainerOperationException();
    }

    return name;
}

} // namespace

const std::uint64_t DEFAULT_CPU_SHARES = 1024;
const std::uint64_t DEFAULT_VCPU_PERIOD_MS = 100000;

ContainerAdmin::ContainerAdmin(ContainerConfig& config)
    : mConfig(config),
      mDom(utils::readFileContent(mConfig.config)),
      mId(getDomainName(mDom.get())),
      mDetachOnExit(false),
      mLifecycleCallbackId(-1),
      mRebootCallbackId(-1),
      mNextIdForListener(0)
{
    LOGD(mId << ": Instantiating ContainerAdmin object");

    // ContainerAdmin owns those callbacks
    mLifecycleCallbackId = virConnectDomainEventRegisterAny(virDomainGetConnect(mDom.get()),
                                                            mDom.get(),
                                                            VIR_DOMAIN_EVENT_ID_LIFECYCLE,
                                                            VIR_DOMAIN_EVENT_CALLBACK(&ContainerAdmin::libvirtLifecycleCallback),
                                                            utils::createCallbackWrapper(this, mLibvirtGuard.spawn()),
                                                            &utils::deleteCallbackWrapper<ContainerAdmin*>);

    if (mLifecycleCallbackId < 0) {
        LOGE(mId << ": Failed to register a libvirt lifecycle callback");
        throw ContainerOperationException(mId + ": Failed to register a libvirt lifecycle callback");
    }

    mRebootCallbackId = virConnectDomainEventRegisterAny(virDomainGetConnect(mDom.get()),
                                                         mDom.get(),
                                                         VIR_DOMAIN_EVENT_ID_REBOOT,
                                                         VIR_DOMAIN_EVENT_CALLBACK(&ContainerAdmin::libvirtRebootCallback),
                                                         utils::createCallbackWrapper(this, mLibvirtGuard.spawn()),
                                                         &utils::deleteCallbackWrapper<ContainerAdmin*>);

    if (mRebootCallbackId < 0) {
        LOGE(mId << ": Failed to register a libvirt reboot callback");
        virConnectDomainEventDeregisterAny(virDomainGetConnect(mDom.get()),
                                           mLifecycleCallbackId);
        throw ContainerOperationException(mId + ": Failed to register a libvirt reboot callback");
    }
}


ContainerAdmin::~ContainerAdmin()
{
    LOGD(mId << ": Destroying ContainerAdmin object...");

    // Deregister callbacks
    if (mLifecycleCallbackId >= 0) {
        virConnectDomainEventDeregisterAny(virDomainGetConnect(mDom.get()),
                                           mLifecycleCallbackId);
    }
    if (mRebootCallbackId >= 0) {
        virConnectDomainEventDeregisterAny(virDomainGetConnect(mDom.get()),
                                           mRebootCallbackId);
    }

    // Try to forcefully stop
    if (!mDetachOnExit) {
        try {
            destroy();
        } catch (ServerException&) {
            LOGE(mId << ": Failed to destroy the container");
        }
    }

    LOGD(mId << ": ContainerAdmin object destroyed");
}


const std::string& ContainerAdmin::getId() const
{
    return mId;
}


void ContainerAdmin::start()
{
    assert(mDom);

    LOGD(mId << ": Starting...");
    if (isRunning()) {
        LOGD(mId << ": Already running - nothing to do...");
        return;
    }

    // In order to update daemon without shutting down the containers
    // autodestroy option must NOT be set. It's best to create domain
    // without any flags.
    u_int flags = VIR_DOMAIN_NONE;

    if (virDomainCreateWithFlags(mDom.get(), flags) < 0) {
        LOGE(mId << ": Failed to start the container\n"
             << libvirt::libvirtFormatError());
        throw ContainerOperationException();
    }

    LOGD(mId << ": Started");
}


void ContainerAdmin::stop()
{
    assert(mDom);

    LOGD(mId << ": Stopping procedure started...");
    if (isStopped()) {
        LOGD(mId << ": Already crashed/down/off - nothing to do");
        return;
    }

    utils::Latch stoppedOccured;

    LifecycleListener setStopped = [&](const int eventId, const int detailId) {
        if (eventId == VIR_DOMAIN_EVENT_STOPPED) {
            if (detailId != VIR_DOMAIN_EVENT_STOPPED_SHUTDOWN) {
                LOGW(mId << ": shutdown requested, but the container stopped with a different status: "
                     << libvirt::libvirtEventDetailToString(eventId, detailId));
            }
            stoppedOccured.set();
        }
    };

    ListenerId id = registerListener(setStopped, nullptr);
    shutdown();
    bool stopped = stoppedOccured.wait(SHUTDOWN_WAIT);
    removeListener<LifecycleListener>(id);

    if (!stopped) {
        LOGW(mId << ": Gracefull shutdown timed out, the container is still running, destroying");
        destroy();
    }

    LOGD(mId << ": Stopping procedure ended");
}


void ContainerAdmin::destroy()
{
    assert(mDom);

    LOGD(mId << ": Destroying...");
    if (isStopped()) {
        LOGD(mId << ": Already crashed/down/off - nothing to do");
        return;
    }

    setSchedulerLevel(SchedulerLevel::FOREGROUND);

    // Forceful termination of the guest
    u_int flags = VIR_DOMAIN_DESTROY_DEFAULT;

    if (virDomainDestroyFlags(mDom.get(), flags) < 0) {
        LOGE(mId << ": Error while destroying the container:\n"
             << libvirt::libvirtFormatError());
        throw ContainerOperationException();
    }

    LOGD(mId << ": Destroyed");
}


void ContainerAdmin::shutdown()
{
    assert(mDom);

    LOGD(mId << ": Shutting down...");
    if (isStopped()) {
        LOGD(mId << ": Already crashed/down/off - nothing to do");
        return;
    }

    setSchedulerLevel(SchedulerLevel::FOREGROUND);

    if (virDomainShutdown(mDom.get()) < 0) {
        LOGE(mId << ": Error while shutting down the container:\n"
             << libvirt::libvirtFormatError());
        throw ContainerOperationException();
    }

    LOGD(mId << ": Shut down initiated (async)");
}


bool ContainerAdmin::isRunning()
{
    return getState() == VIR_DOMAIN_RUNNING;
}


bool ContainerAdmin::isStopped()
{
    int state = getState();
    return state == VIR_DOMAIN_SHUTDOWN ||
           state == VIR_DOMAIN_SHUTOFF ||
           state == VIR_DOMAIN_CRASHED;
}


void ContainerAdmin::suspend()
{
    assert(mDom);

    LOGD(mId << ": Pausing...");
    if (isPaused()) {
        LOGD(mId << ": Already paused - nothing to do...");
        return;
    }

    if (virDomainSuspend(mDom.get()) < 0) {
        LOGE(mId << ": Error while suspending the container:\n"
             << libvirt::libvirtFormatError());
        throw ContainerOperationException();
    }

    LOGD(mId << ": Paused");
}


void ContainerAdmin::resume()
{
    assert(mDom);

    LOGD(mId << ": Resuming...");
    if (!isPaused()) {
        LOGD(mId << ": Is not paused - nothing to do...");
        return;
    }

    if (virDomainResume(mDom.get()) < 0) {
        LOGE(mId << ": Error while resuming the container:\n"
             << libvirt::libvirtFormatError());
        throw ContainerOperationException();
    }

    LOGD(mId << ": Resumed");
}


bool ContainerAdmin::isPaused()
{
    return getState() == VIR_DOMAIN_PAUSED;
}


int ContainerAdmin::getState()
{
    assert(mDom);

    int state;

    if (virDomainGetState(mDom.get(), &state, NULL, 0)) {
        LOGE(mId << ": Error while getting the container's state:\n"
             << libvirt::libvirtFormatError());
        throw ContainerOperationException();
    }

    return state;
}


void ContainerAdmin::setSchedulerLevel(SchedulerLevel sched)
{
    switch (sched) {
    case SchedulerLevel::FOREGROUND:
        LOGD(mId << ": Setting SchedulerLevel::FOREGROUND");
        setSchedulerParams(DEFAULT_CPU_SHARES,
                           DEFAULT_VCPU_PERIOD_MS,
                           mConfig.cpuQuotaForeground);
        break;
    case SchedulerLevel::BACKGROUND:
        LOGD(mId << ": Setting SchedulerLevel::BACKGROUND");
        setSchedulerParams(DEFAULT_CPU_SHARES,
                           DEFAULT_VCPU_PERIOD_MS,
                           mConfig.cpuQuotaBackground);
        break;
    default:
        assert(0 && "Unknown sched parameter value");
    }
}


void ContainerAdmin::setSchedulerParams(std::uint64_t cpuShares, std::uint64_t vcpuPeriod, std::int64_t vcpuQuota)
{
    assert(mDom);

    int maxParams = 3;
    int numParamsBuff = 0;

    std::unique_ptr<virTypedParameter[]> params(new virTypedParameter[maxParams]);

    virTypedParameterPtr paramsTmp = params.get();

    virTypedParamsAddULLong(&paramsTmp, &numParamsBuff, &maxParams, VIR_DOMAIN_SCHEDULER_CPU_SHARES, cpuShares);
    virTypedParamsAddULLong(&paramsTmp, &numParamsBuff, &maxParams, VIR_DOMAIN_SCHEDULER_VCPU_PERIOD, vcpuPeriod);
    virTypedParamsAddLLong(&paramsTmp, &numParamsBuff, &maxParams, VIR_DOMAIN_SCHEDULER_VCPU_QUOTA, vcpuQuota);

    if (virDomainSetSchedulerParameters(mDom.get(), params.get(), numParamsBuff) < 0) {
        LOGE(mId << ": Error while setting the container's scheduler params:\n"
             << libvirt::libvirtFormatError());
        throw ContainerOperationException();
    }
}

void ContainerAdmin::setDetachOnExit()
{
    mDetachOnExit = true;
}

std::int64_t ContainerAdmin::getSchedulerQuota()
{
    assert(mDom);

    int numParamsBuff;
    std::unique_ptr<char, void(*)(void*)> type(virDomainGetSchedulerType(mDom.get(), &numParamsBuff), free);

    if (type == NULL || numParamsBuff <= 0 || strcmp(type.get(), "posix") != 0) {
        LOGE(mId << ": Error while getting the container's scheduler type:\n"
             << libvirt::libvirtFormatError());
        throw ContainerOperationException();
    }

    std::unique_ptr<virTypedParameter[]> params(new virTypedParameter[numParamsBuff]);

    if (virDomainGetSchedulerParameters(mDom.get(), params.get(), &numParamsBuff) < 0) {
        LOGE(mId << ": Error while getting the container's scheduler params:\n"
             << libvirt::libvirtFormatError());
        throw ContainerOperationException();
    }

    long long quota;
    if (virTypedParamsGetLLong(params.get(),
                               numParamsBuff,
                               VIR_DOMAIN_SCHEDULER_VCPU_QUOTA,
                               &quota) <= 0) {
        LOGE(mId << ": Error while getting the container's scheduler quota param:\n"
             << libvirt::libvirtFormatError());
        throw ContainerOperationException();
    }

    return quota;
}

int ContainerAdmin::libvirtLifecycleCallback(virConnectPtr /*con*/,
                                             virDomainPtr /*dom*/,
                                             int event,
                                             int detail,
                                             void* opaque)
{
    ContainerAdmin* thisPtr = utils::getCallbackFromPointer<ContainerAdmin*>(opaque);

    LOGI(thisPtr->getId()
         << ": Lifecycle event: "
         << libvirt::libvirtEventToString(event)
         << ": "
         << libvirt::libvirtEventDetailToString(event, detail));

    std::unique_lock<std::mutex> lock(thisPtr->mListenerMutex);
    for (auto& it : thisPtr->mLifecycleListeners) {
        LifecycleListener f = it.second.get();
        f(event, detail);
    }

    // ignored, libvirt's legacy
    return 0;
}

void ContainerAdmin::libvirtRebootCallback(virConnectPtr /*con*/,
                                           virDomainPtr /*dom*/,
                                           void* opaque)
{
    ContainerAdmin* thisPtr = utils::getCallbackFromPointer<ContainerAdmin*>(opaque);

    LOGI(thisPtr->getId() << ": Reboot event");

    std::unique_lock<std::mutex> lock(thisPtr->mListenerMutex);
    for (auto& it : thisPtr->mRebootListeners) {
        RebootListener f = it.second.get();
        f();
    }
}

template<>
ContainerAdmin::ListenerMap<ContainerAdmin::LifecycleListener>&
ContainerAdmin::getListenerMap<ContainerAdmin::LifecycleListener>()
{
    return mLifecycleListeners;
}

template<>
ContainerAdmin::ListenerMap<ContainerAdmin::RebootListener>&
ContainerAdmin::getListenerMap<ContainerAdmin::RebootListener>()
{
    return mRebootListeners;
}


} // namespace security_containers
