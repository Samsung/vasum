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

#include "config.hpp"

#include "container-admin.hpp"
#include "exception.hpp"

#include "logger/logger.hpp"
#include "utils/paths.hpp"

#include <cassert>


namespace security_containers {

namespace {

// TODO: this should be in container's configuration file
const int SHUTDOWN_WAIT = 10;

class Args {
public:
    Args(const std::vector<std::string>& args)
    {
        mArgs.reserve(args.size() + 1);
        for (const std::string& arg : args) {
            mArgs.push_back(arg.c_str());
        }
        mArgs.push_back(NULL);
    }
    bool empty() const
    {
        return mArgs.size() == 1;
    }
    const char* const* getAsCArray() const
    {
        return mArgs.data();
    }
    friend std::ostream& operator<<(std::ostream& os, const Args& a)
    {
        for (const char* arg : a.mArgs) {
            if (arg != NULL) {
                os << "'" << arg << "' ";
            }
        }
        return os;
    }
private:
    std::vector<const char*> mArgs;
};

} // namespace

const std::uint64_t DEFAULT_CPU_SHARES = 1024;
const std::uint64_t DEFAULT_VCPU_PERIOD_MS = 100000;

ContainerAdmin::ContainerAdmin(const std::string& containersPath,
                               const std::string& lxcTemplatePrefix,
                               const ContainerConfig& config)
    : mConfig(config),
      mDom(containersPath, config.name),
      mId(mDom.getName()),
      mDetachOnExit(false)
{
    LOGD(mId << ": Instantiating ContainerAdmin object");

    if (!mDom.isDefined()) {

        const std::string lxcTemplate = utils::getAbsolutePath(config.lxcTemplate,
                                                               lxcTemplatePrefix);
        LOGI(mId << ": Creating domain from template: " << lxcTemplate);
        std::vector<std::string> args;
        if (!config.ipv4Gateway.empty()) {
            args.push_back("--ipv4-gateway");
            args.push_back(config.ipv4Gateway);
        }
        if (!config.ipv4.empty()) {
            args.push_back("--ipv4");
            args.push_back(config.ipv4);
        }
        if (!mDom.create(lxcTemplate, Args(args).getAsCArray())) {
            throw ContainerOperationException("Could not create domain");
        }
    }
}


ContainerAdmin::~ContainerAdmin()
{
    LOGD(mId << ": Destroying ContainerAdmin object...");

    if (!mDetachOnExit) {
        // Try to forcefully stop
        if (!mDom.stop()) {
            LOGE(mId << ": Failed to stop the container");
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
    LOGD(mId << ": Starting...");
    if (isRunning()) {
        LOGD(mId << ": Already running - nothing to do...");
        return;
    }

    const Args args(mConfig.initWithArgs);
    bool result;
    if (args.empty()) {
        result = mDom.start(NULL);
    } else {
        LOGD(mId << ": Init: " << args);
        result = mDom.start(args.getAsCArray());
    }

    if (!result) {
        throw ContainerOperationException("Could not start container");
    }

    LOGD(mId << ": Started");
}


void ContainerAdmin::stop()
{
    LOGD(mId << ": Stopping procedure started...");
    if (isStopped()) {
        LOGD(mId << ": Already crashed/down/off - nothing to do");
        return;
    }

    if (!mDom.shutdown(SHUTDOWN_WAIT)) {
        // force stop
        if (!mDom.stop()) {
            throw ContainerOperationException("Could not stop container");
        }
    }

    LOGD(mId << ": Stopping procedure ended");
}


void ContainerAdmin::destroy()
{
    LOGD(mId << ": Destroying procedure started...");

    if (!mDom.destroy()) {
        throw ContainerOperationException("Could not destroy container");
    }

    LOGD(mId << ": Destroying procedure ended");
}


bool ContainerAdmin::isRunning()
{
    return mDom.getState() == lxc::LxcDomain::State::RUNNING;
}


bool ContainerAdmin::isStopped()
{
    return mDom.getState() == lxc::LxcDomain::State::STOPPED;
}


void ContainerAdmin::suspend()
{
    LOGD(mId << ": Pausing...");
    if (!mDom.freeze()) {
        throw ContainerOperationException("Could not pause container");
    }
    LOGD(mId << ": Paused");
}


void ContainerAdmin::resume()
{
    LOGD(mId << ": Resuming...");
    if (!mDom.unfreeze()) {
        throw ContainerOperationException("Could not resume container");
    }
    LOGD(mId << ": Resumed");
}


bool ContainerAdmin::isPaused()
{
    return mDom.getState() == lxc::LxcDomain::State::FROZEN;
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


void ContainerAdmin::setSchedulerParams(std::uint64_t, std::uint64_t, std::int64_t)
//void ContainerAdmin::setSchedulerParams(std::uint64_t cpuShares, std::uint64_t vcpuPeriod, std::int64_t vcpuQuota)
{
//    assert(mDom);
//
//    int maxParams = 3;
//    int numParamsBuff = 0;
//
//    std::unique_ptr<virTypedParameter[]> params(new virTypedParameter[maxParams]);
//
//    virTypedParameterPtr paramsTmp = params.get();
//
//    virTypedParamsAddULLong(&paramsTmp, &numParamsBuff, &maxParams, VIR_DOMAIN_SCHEDULER_CPU_SHARES, cpuShares);
//    virTypedParamsAddULLong(&paramsTmp, &numParamsBuff, &maxParams, VIR_DOMAIN_SCHEDULER_VCPU_PERIOD, vcpuPeriod);
//    virTypedParamsAddLLong(&paramsTmp, &numParamsBuff, &maxParams, VIR_DOMAIN_SCHEDULER_VCPU_QUOTA, vcpuQuota);
//
//    if (virDomainSetSchedulerParameters(mDom.get(), params.get(), numParamsBuff) < 0) {
//        LOGE(mId << ": Error while setting the container's scheduler params:\n"
//             << libvirt::libvirtFormatError());
//        throw ContainerOperationException();
//    }
}

void ContainerAdmin::setDetachOnExit()
{
    mDetachOnExit = true;
}

std::int64_t ContainerAdmin::getSchedulerQuota()
{
//    assert(mDom);
//
//    int numParamsBuff;
//    std::unique_ptr<char, void(*)(void*)> type(virDomainGetSchedulerType(mDom.get(), &numParamsBuff), free);
//
//    if (type == NULL || numParamsBuff <= 0 || strcmp(type.get(), "posix") != 0) {
//        LOGE(mId << ": Error while getting the container's scheduler type:\n"
//             << libvirt::libvirtFormatError());
//        throw ContainerOperationException();
//    }
//
//    std::unique_ptr<virTypedParameter[]> params(new virTypedParameter[numParamsBuff]);
//
//    if (virDomainGetSchedulerParameters(mDom.get(), params.get(), &numParamsBuff) < 0) {
//        LOGE(mId << ": Error while getting the container's scheduler params:\n"
//             << libvirt::libvirtFormatError());
//        throw ContainerOperationException();
//    }
//
//    long long quota;
//    if (virTypedParamsGetLLong(params.get(),
//                               numParamsBuff,
//                               VIR_DOMAIN_SCHEDULER_VCPU_QUOTA,
//                               &quota) <= 0) {
//        LOGE(mId << ": Error while getting the container's scheduler quota param:\n"
//             << libvirt::libvirtFormatError());
//        throw ContainerOperationException();
//    }
//
//    return quota;
    return 0;
}


} // namespace security_containers
