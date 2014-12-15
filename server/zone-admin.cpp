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
 * @brief   Implementation of class for administrating one zone
 */

#include "config.hpp"

#include "zone-admin.hpp"
#include "exception.hpp"

#include "logger/logger.hpp"
#include "utils/paths.hpp"
#include "utils/c-array.hpp"

#include <cassert>


namespace vasum {

namespace {

// TODO: this should be in zone's configuration file
const int SHUTDOWN_WAIT = 10;

} // namespace

const std::uint64_t DEFAULT_CPU_SHARES = 1024;
const std::uint64_t DEFAULT_VCPU_PERIOD_MS = 100000;

ZoneAdmin::ZoneAdmin(const std::string& zonesPath,
                               const std::string& lxcTemplatePrefix,
                               const ZoneConfig& config)
    : mConfig(config),
      mZone(zonesPath, config.name),
      mId(mZone.getName()),
      mDetachOnExit(false),
      mDestroyOnExit(false)
{
    LOGD(mId << ": Instantiating ZoneAdmin object");

    if (!mZone.isDefined()) {

        const std::string lxcTemplate = utils::getAbsolutePath(config.lxcTemplate,
                                                               lxcTemplatePrefix);
        LOGI(mId << ": Creating zone from template: " << lxcTemplate);
        utils::CStringArrayBuilder args;
        if (!config.ipv4Gateway.empty()) {
            args.add("--ipv4-gateway");
            args.add(config.ipv4Gateway.c_str());
        }
        if (!config.ipv4.empty()) {
            args.add("--ipv4");
            args.add(config.ipv4.c_str());
        }
        const std::string vt = std::to_string(config.vt);
        if (config.vt > 0) {
            args.add("--vt");
            args.add(vt.c_str());
        }
        if (!mZone.create(lxcTemplate, args.c_array())) {
            throw ZoneOperationException("Could not create zone");
        }
    }
}


ZoneAdmin::~ZoneAdmin()
{
    LOGD(mId << ": Destroying ZoneAdmin object...");

    if (mDestroyOnExit) {
        if (!mZone.stop()) {
            LOGE(mId << ": Failed to stop the zone");
        }
        if (!mZone.destroy()) {
            LOGE(mId << ": Failed to destroy the zone");
        }
    }

    if (!mDetachOnExit) {
        // Try to forcefully stop
        if (!mZone.stop()) {
            LOGE(mId << ": Failed to stop the zone");
        }
    }

    LOGD(mId << ": ZoneAdmin object destroyed");
}


const std::string& ZoneAdmin::getId() const
{
    return mId;
}


void ZoneAdmin::start()
{
    LOGD(mId << ": Starting...");
    if (isRunning()) {
        LOGD(mId << ": Already running - nothing to do...");
        return;
    }

    utils::CStringArrayBuilder args;
    for (const std::string& arg : mConfig.initWithArgs) {
        args.add(arg.c_str());
    }
    if (args.empty()) {
        args.add("/sbin/init");
    }

    if (!mZone.start(args.c_array())) {
        throw ZoneOperationException("Could not start zone");
    }

    LOGD(mId << ": Started");
}


void ZoneAdmin::stop()
{
    LOGD(mId << ": Stopping procedure started...");
    if (isStopped()) {
        LOGD(mId << ": Already crashed/down/off - nothing to do");
        return;
    }

    if (!mZone.shutdown(SHUTDOWN_WAIT)) {
        // force stop
        if (!mZone.stop()) {
            throw ZoneOperationException("Could not stop zone");
        }
    }

    LOGD(mId << ": Stopping procedure ended");
}


void ZoneAdmin::destroy()
{
    LOGD(mId << ": Destroying procedure started...");

    if (!mZone.destroy()) {
        throw ZoneOperationException("Could not destroy zone");
    }

    LOGD(mId << ": Destroying procedure ended");
}


bool ZoneAdmin::isRunning()
{
    return mZone.getState() == lxc::LxcZone::State::RUNNING;
}


bool ZoneAdmin::isStopped()
{
    return mZone.getState() == lxc::LxcZone::State::STOPPED;
}


void ZoneAdmin::suspend()
{
    LOGD(mId << ": Pausing...");
    if (!mZone.freeze()) {
        throw ZoneOperationException("Could not pause zone");
    }
    LOGD(mId << ": Paused");
}


void ZoneAdmin::resume()
{
    LOGD(mId << ": Resuming...");
    if (!mZone.unfreeze()) {
        throw ZoneOperationException("Could not resume zone");
    }
    LOGD(mId << ": Resumed");
}


bool ZoneAdmin::isPaused()
{
    return mZone.getState() == lxc::LxcZone::State::FROZEN;
}


void ZoneAdmin::setSchedulerLevel(SchedulerLevel sched)
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


void ZoneAdmin::setSchedulerParams(std::uint64_t, std::uint64_t, std::int64_t)
//void ZoneAdmin::setSchedulerParams(std::uint64_t cpuShares, std::uint64_t vcpuPeriod, std::int64_t vcpuQuota)
{
//    assert(mZone);
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
//    if (virDomainSetSchedulerParameters(mZone.get(), params.get(), numParamsBuff) < 0) {
//        LOGE(mId << ": Error while setting the zone's scheduler params:\n"
//             << libvirt::libvirtFormatError());
//        throw ZoneOperationException();
//    }
}

void ZoneAdmin::setDetachOnExit()
{
    mDetachOnExit = true;
}

void ZoneAdmin::setDestroyOnExit()
{
    mDestroyOnExit = true;
}

std::int64_t ZoneAdmin::getSchedulerQuota()
{
//    assert(mZone);
//
//    int numParamsBuff;
//    std::unique_ptr<char, void(*)(void*)> type(virDomainGetSchedulerType(mZone.get(), &numParamsBuff), free);
//
//    if (type == NULL || numParamsBuff <= 0 || strcmp(type.get(), "posix") != 0) {
//        LOGE(mId << ": Error while getting the zone's scheduler type:\n"
//             << libvirt::libvirtFormatError());
//        throw ZoneOperationException();
//    }
//
//    std::unique_ptr<virTypedParameter[]> params(new virTypedParameter[numParamsBuff]);
//
//    if (virDomainGetSchedulerParameters(mZone.get(), params.get(), &numParamsBuff) < 0) {
//        LOGE(mId << ": Error while getting the zone's scheduler params:\n"
//             << libvirt::libvirtFormatError());
//        throw ZoneOperationException();
//    }
//
//    long long quota;
//    if (virTypedParamsGetLLong(params.get(),
//                               numParamsBuff,
//                               VIR_DOMAIN_SCHEDULER_VCPU_QUOTA,
//                               &quota) <= 0) {
//        LOGE(mId << ": Error while getting the zone's scheduler quota param:\n"
//             << libvirt::libvirtFormatError());
//        throw ZoneOperationException();
//    }
//
//    return quota;
    return 0;
}


} // namespace vasum
