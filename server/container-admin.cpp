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

#include "log/logger.hpp"
#include "utils/fs.hpp"

#include <cassert>
#include <cstring>
#include <string>
#include <memory>
#include <cstdint>


namespace security_containers {


const std::uint64_t DEFAULT_CPU_SHARES = 1024;
const std::uint64_t DEFAULT_VCPU_PERIOD_MS = 100000;

ContainerAdmin::ContainerAdmin(ContainerConfig& config)
    : mConfig(config), mDom(utils::readFileContent(mConfig.config))
{
}


ContainerAdmin::~ContainerAdmin()
{
    // Try to shutdown
    try {
        shutdown();
    } catch (ServerException& e) {}

    // Try to forcefully stop
    try {
        stop();
    } catch (ServerException& e) {
        LOGE("Failed to destroy the container!");
    }
}


std::string ContainerAdmin::getId()
{
    assert(mDom.get() != NULL);

    const char* id;
    if ((id = virDomainGetName(mDom.get())) == NULL) {
        LOGE("Failed to get container's id");
        throw DomainOperationException();
    }

    return id;
}


void ContainerAdmin::start()
{
    assert(mDom.get() != NULL);

    if (isRunning()) {
        return;
    }

    // Autodestroyed when connection pointer released
    // Any managed save file for this domain is discarded,
    // and the domain boots from scratch
    u_int flags = VIR_DOMAIN_START_AUTODESTROY;

    if (virDomainCreateWithFlags(mDom.get(), flags) < 0) {
        LOGE("Failed to start the container");
        throw DomainOperationException();
    }

    // TODO: the container should be started in the background,
    // unfortunately libvirt doesn't allow us to set cgroups
    // before the start, hence we do it immediately afterwards
    setSchedulerLevel(SchedulerLevel::BACKGROUND);
}


void ContainerAdmin::stop()
{
    assert(mDom.get() != NULL);

    if (isStopped()) {
        return;
    }

    // Forceful termination of the guest
    u_int flags = VIR_DOMAIN_DESTROY_DEFAULT;

    if (virDomainDestroyFlags(mDom.get(), flags) < 0) {
        LOGE("Error during domain stopping");
        throw DomainOperationException();
    }
}


void ContainerAdmin::shutdown()
{
    assert(mDom.get() != NULL);

    if (isStopped()) {
        return;
    }

    resume();

    if (virDomainShutdown(mDom.get()) < 0) {
        LOGE("Error during domain shutdown");
        throw DomainOperationException();
    }
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
    assert(mDom.get() != NULL);

    if (isPaused()) {
        return;
    }

    if (virDomainSuspend(mDom.get()) < 0) {
        LOGE("Error during domain suspension");
        throw DomainOperationException();
    }
}


void ContainerAdmin::resume()
{
    assert(mDom.get() != NULL);

    if (!isPaused()) {
        return;
    }

    if (virDomainResume(mDom.get()) < 0) {
        LOGE("Error during domain resumming");
        throw DomainOperationException();
    }
}


bool ContainerAdmin::isPaused()
{
    return getState() == VIR_DOMAIN_PAUSED;
}


int ContainerAdmin::getState()
{
    assert(mDom.get() != NULL);

    int state;

    if (virDomainGetState(mDom.get(), &state, NULL, 0)) {
        LOGE("Error during getting domain's state");
        throw DomainOperationException();
    }

    return state;
}


void ContainerAdmin::setSchedulerLevel(SchedulerLevel sched)
{
    switch (sched) {
    case SchedulerLevel::FOREGROUND:
        setSchedulerParams(DEFAULT_CPU_SHARES,
                           DEFAULT_VCPU_PERIOD_MS,
                           mConfig.cpuQuotaForeground);
        break;
    case SchedulerLevel::BACKGROUND:
        setSchedulerParams(DEFAULT_CPU_SHARES,
                           DEFAULT_VCPU_PERIOD_MS,
                           mConfig.cpuQuotaBackground);
        break;
    default:
        assert(!"Unknown sched parameter value");
    }
}


void ContainerAdmin::setSchedulerParams(std::uint64_t cpuShares, std::uint64_t vcpuPeriod, std::int64_t vcpuQuota)
{
    assert(mDom.get() != NULL);

    int maxParams = 3;
    int numParamsBuff = 0;

    std::unique_ptr<virTypedParameter[]> params(new virTypedParameter[maxParams]);

    virTypedParameterPtr paramsTmp = params.get();

    virTypedParamsAddULLong(&paramsTmp, &numParamsBuff, &maxParams, VIR_DOMAIN_SCHEDULER_CPU_SHARES, cpuShares);
    virTypedParamsAddULLong(&paramsTmp, &numParamsBuff, &maxParams, VIR_DOMAIN_SCHEDULER_VCPU_PERIOD, vcpuPeriod);
    virTypedParamsAddLLong(&paramsTmp, &numParamsBuff, &maxParams, VIR_DOMAIN_SCHEDULER_VCPU_QUOTA, vcpuQuota);

    if (virDomainSetSchedulerParameters(mDom.get(), params.get(), numParamsBuff) < 0) {
        LOGE("Error whilte setting scheduler params");
        throw DomainOperationException();
    }
}


std::int64_t ContainerAdmin::getSchedulerQuota()
{
    assert(mDom.get() != NULL);

    int numParamsBuff;
    std::unique_ptr<char, void(*)(void*)> type(virDomainGetSchedulerType(mDom.get(), &numParamsBuff), free);

    if (type == NULL || numParamsBuff <= 0 || strcmp(type.get(), "posix") != 0) {
        LOGE("Error while getting scheduler type");
        throw DomainOperationException();
    }

    std::unique_ptr<virTypedParameter[]> params(new virTypedParameter[numParamsBuff]);

    if (virDomainGetSchedulerParameters(mDom.get(), params.get(), &numParamsBuff) < 0) {
        LOGE("Error whilte getting scheduler parameters");
        throw DomainOperationException();
    }

    long long quota;
    if (virTypedParamsGetLLong(params.get(),
                               numParamsBuff,
                               VIR_DOMAIN_SCHEDULER_VCPU_QUOTA,
                               &quota) <= 0) {
        LOGE("Error whilte getting scheduler quota parameter");
        throw DomainOperationException();
    }

    return quota;
}


} // namespace security_containers
