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
#include "netdev.hpp"

#include "logger/logger.hpp"
#include "utils/paths.hpp"
#include "utils/c-array.hpp"
#include "lxc/cgroup.hpp"

#include <cassert>
#include <climits>
#include <thread>
#include <chrono>


namespace vasum {

namespace {

// TODO: this should be in zone's configuration file
const int SHUTDOWN_WAIT = 10;

} // namespace

const std::uint64_t DEFAULT_CPU_SHARES = 1024;
const std::uint64_t DEFAULT_VCPU_PERIOD_MS = 100000;

ZoneAdmin::ZoneAdmin(const std::string& zoneId,
                     const std::string& zonesPath,
                     const std::string& lxcTemplatePrefix,
                     const ZoneConfig& config,
                     const ZoneDynamicConfig& dynamicConfig)
    : mConfig(config),
      mDynamicConfig(dynamicConfig),
      mZone(zonesPath, zoneId),
      mId(zoneId),
      mDetachOnExit(false),
      mDestroyOnExit(false)
{
    LOGD(mId << ": Instantiating ZoneAdmin object");

    if (!mZone.isDefined()) {

        const std::string lxcTemplate = utils::getAbsolutePath(config.lxcTemplate,
                                                               lxcTemplatePrefix);
        LOGI(mId << ": Creating zone from template: " << lxcTemplate);
        utils::CStringArrayBuilder args;
        if (!dynamicConfig.ipv4Gateway.empty()) {
            args.add("--ipv4-gateway");
            args.add(dynamicConfig.ipv4Gateway.c_str());
        }
        if (!dynamicConfig.ipv4.empty()) {
            args.add("--ipv4");
            args.add(dynamicConfig.ipv4.c_str());
        }
        const std::string vt = std::to_string(dynamicConfig.vt);
        if (dynamicConfig.vt > 0) {
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

    // Wait until the full platform launch with graphical stack.
    // VT should be activated by a graphical stack.
    // If we do it with 'zoneToFocus.activateVT' before starting the graphical stack,
    // graphical stack initialization failed and we finally switch to the black screen.
    // Skip waiting when graphical stack is not running (unit tests).
    if (mDynamicConfig.vt > 0) {
        // TODO, timeout is a temporary solution
        std::this_thread::sleep_for(std::chrono::milliseconds(4000));
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
    assert(isRunning());

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


void ZoneAdmin::setSchedulerParams(std::uint64_t cpuShares,
                                   std::uint64_t vcpuPeriod,
                                   std::int64_t vcpuQuota)
{
    assert(vcpuPeriod >= 1000 && vcpuPeriod <= 1000000);
    assert(vcpuQuota == -1 ||
           (vcpuQuota >= 1000 && vcpuQuota <= static_cast<std::int64_t>(ULLONG_MAX / 1000)));

    if (!lxc::setCgroup(mId, "cpu", "cpu.shares", std::to_string(cpuShares)) ||
        !lxc::setCgroup(mId, "cpu", "cpu.cfs_period_us", std::to_string(vcpuPeriod)) ||
        !lxc::setCgroup(mId, "cpu", "cpu.cfs_quota_us", std::to_string(vcpuQuota))) {

        LOGE(mId << ": Error while setting the zone's scheduler params");
        throw ZoneOperationException("Could not set scheduler params");
    }
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
    std::string ret;
    if (!lxc::getCgroup(mId, "cpu", "cpu.cfs_quota_us", ret)) {
        LOGE(mId << ": Error while getting the zone's scheduler quota param");
        throw ZoneOperationException("Could not get scheduler quota param");
    }
    return std::stoll(ret);
}

void ZoneAdmin::createNetdevVeth(const std::string& zoneDev,
                                 const std::string& hostDev)
{
    netdev::createVeth(mZone.getInitPid(), zoneDev, hostDev);
}

void ZoneAdmin::createNetdevMacvlan(const std::string& zoneDev,
                                    const std::string& hostDev,
                                    const uint32_t& mode)
{
    netdev::createMacvlan(mZone.getInitPid(), zoneDev, hostDev, static_cast<macvlan_mode>(mode));
}

void ZoneAdmin::moveNetdev(const std::string& devId)
{
    netdev::movePhys(mZone.getInitPid(), devId);
}

void ZoneAdmin::destroyNetdev(const std::string& devId)
{
    netdev::destroyNetdev(devId, mZone.getInitPid());
}

void ZoneAdmin::setNetdevAttrs(const std::string& netdev, const NetdevAttrs& attrs)
{
    netdev::setAttrs(mZone.getInitPid(), netdev, attrs);
}

ZoneAdmin::NetdevAttrs ZoneAdmin::getNetdevAttrs(const std::string& netdev)
{
    return netdev::getAttrs(mZone.getInitPid(), netdev);
}

std::vector<std::string> ZoneAdmin::getNetdevList()
{
    return netdev::listNetdev(mZone.getInitPid());
}

} // namespace vasum
