/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Lukasz Pawelczyk <l.pawelczyk@partner.samsung.com>
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
 * @author  Lukasz Pawelczyk (l.pawelczyk@partner.samsung.com)
 * @brief   Implementation of class for managing one zone
 */

#include "config.hpp"

#include "zone.hpp"
#include "dynamic-config-scheme.hpp"
#include "exception.hpp"

#include "logger/logger.hpp"
#include "utils/paths.hpp"
#include "utils/vt.hpp"
#include "utils/c-array.hpp"
#include "lxc/cgroup.hpp"
#include "config/manager.hpp"

#include <boost/filesystem.hpp>

#include <cassert>
#include <climits>
#include <string>
#include <thread>
#include <chrono>

namespace vasum {

namespace fs = boost::filesystem;

namespace {

typedef std::lock_guard<std::recursive_mutex> Lock;

const std::string STATE_STOPPED = "stopped";
const std::string STATE_RUNNING = "running";
const std::string STATE_PAUSED = "paused";

const std::uint64_t DEFAULT_CPU_SHARES = 1024;
const std::uint64_t DEFAULT_VCPU_PERIOD_MS = 100000;

} // namespace

Zone::Zone(const std::string& zoneId,
           const std::string& zonesPath,
           const std::string& zoneTemplatePath,
           const std::string& dbPath,
           const std::string& zoneTemplateDir,
           const std::string& baseRunMountPointPath)
    : mDbPath(dbPath)
    , mZone(zonesPath, zoneId)
    , mId(zoneId)
    , mDetachOnExit(false)
    , mDestroyOnExit(false)
{
    LOGD(mId << ": Instantiating Zone object");

    const std::string dbPrefix = getZoneDbPrefix(zoneId);
    config::loadFromKVStoreWithJsonFile(dbPath, zoneTemplatePath, mConfig, dbPrefix);
    config::loadFromKVStoreWithJsonFile(dbPath, zoneTemplatePath, mDynamicConfig, dbPrefix);

    for (std::string r: mConfig.permittedToSend) {
        mPermittedToSend.push_back(boost::regex(r));
    }
    for (std::string r: mConfig.permittedToRecv) {
        mPermittedToRecv.push_back(boost::regex(r));
    }

    if (!mDynamicConfig.runMountPoint.empty()) {
        mRunMountPoint = fs::absolute(mDynamicConfig.runMountPoint, baseRunMountPointPath).string();
    }

    if (!mZone.isDefined()) {
        const std::string zoneTemplate = utils::getAbsolutePath(mConfig.zoneTemplate,
                                                                zoneTemplateDir);
        LOGI(mId << ": Creating zone from template: " << zoneTemplate);
        utils::CStringArrayBuilder args;
        if (!mDynamicConfig.ipv4Gateway.empty()) {
            args.add("--ipv4-gateway");
            args.add(mDynamicConfig.ipv4Gateway.c_str());
        }
        if (!mDynamicConfig.ipv4.empty()) {
            args.add("--ipv4");
            args.add(mDynamicConfig.ipv4.c_str());
        }
        const std::string vt = std::to_string(mDynamicConfig.vt);
        if (mDynamicConfig.vt > 0) {
            args.add("--vt");
            args.add(vt.c_str());
        }
        if (!mZone.create(zoneTemplate, args.c_array())) {
            throw ZoneOperationException("Could not create zone");
        }
    }

    const fs::path zonePath = fs::path(zonesPath) / zoneId;
    mRootPath = (zonePath / fs::path("rootfs")).string();

    mProvision.reset(new ZoneProvision(mRootPath, zoneTemplatePath, dbPath, dbPrefix, mConfig.validLinkPrefixes));
}

Zone::~Zone()
{
    LOGD(mId << ": Destroying Zone object...");

    if (mDestroyOnExit) {
        if (!mZone.stop()) {
            LOGE(mId << ": Failed to stop the zone");
        }
        if (!mZone.destroy()) {
            LOGE(mId << ": Failed to destroy the zone");
        }
    }

    if (!mDetachOnExit && !mDestroyOnExit) {
        // Try to forcefully stop
        if (!mZone.stop()) {
            LOGE(mId << ": Failed to stop the zone");
        }
    }

    LOGD(mId << ": Zone object destroyed");
}

const std::vector<boost::regex>& Zone::getPermittedToSend() const
{
    return mPermittedToSend;
}

const std::vector<boost::regex>& Zone::getPermittedToRecv() const
{
    return mPermittedToRecv;
}

const std::string& Zone::getId() const
{
    Lock lock(mReconnectMutex);
    return mId;
}

int Zone::getPrivilege() const
{
    return mConfig.privilege;
}

void Zone::saveDynamicConfig()
{
    config::saveToKVStore(mDbPath, mDynamicConfig, getZoneDbPrefix(mId));
}

void Zone::updateRequestedState(const std::string& state)
{
    // assume mutex is locked
    if (state != mDynamicConfig.requestedState) {
        LOGT("Set requested state of " << mId << " to " << state);
        mDynamicConfig.requestedState = state;
        saveDynamicConfig();
    }
}

void Zone::restore()
{
    std::string requestedState;
    {
        Lock lock(mReconnectMutex);
        requestedState = mDynamicConfig.requestedState;
        LOGT("Requested state of " << mId << ": " << requestedState);
    }

    if (requestedState == STATE_RUNNING) {
        start();
    } else if (requestedState == STATE_STOPPED) {
    } else if (requestedState == STATE_PAUSED) {
        start();
        suspend();
    } else {
        LOGE("Invalid requested state: " << requestedState);
        assert(0 && "invalid requested state");
    }
}

void Zone::start()
{
    Lock lock(mReconnectMutex);

    LOGD(mId << ": Starting...");

    updateRequestedState(STATE_RUNNING);
    mProvision->start();

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

    // Increase cpu quota before connect, otherwise it'd take ages.
    goForeground();
    // refocus in ZonesManager will adjust cpu quota after all
}

void Zone::stop(bool saveState)
{
    Lock lock(mReconnectMutex);

    LOGD(mId << ": Stopping procedure started...");

    if (saveState) {
        updateRequestedState(STATE_STOPPED);
    }
    if (isRunning()) {
        // boost stopping
        goForeground();
    }

    if (isStopped()) {
        LOGD(mId << ": Already crashed/down/off - nothing to do");
        return;
    }

    if (!mZone.shutdown(mConfig.shutdownTimeout)) {
        // force stop
        if (!mZone.stop()) {
            throw ZoneOperationException("Could not stop zone");
        }
    }

    mProvision->stop();

    LOGD(mId << ": Stopping procedure ended");
}

int Zone::getVT() const
{
    Lock lock(mReconnectMutex);
    return mDynamicConfig.vt;
}

std::string Zone::getRootPath() const
{
    return mRootPath;
}

bool Zone::activateVT()
{
    Lock lock(mReconnectMutex);

    if (mDynamicConfig.vt >= 0) {
        return utils::activateVT(mDynamicConfig.vt);
    }

    return true;
}

void Zone::createNetdevVeth(const std::string& zoneDev,
                            const std::string& hostDev)
{
    Lock lock(mReconnectMutex);
    netdev::createVeth(mZone.getInitPid(), zoneDev, hostDev);
}

void Zone::createNetdevMacvlan(const std::string& zoneDev,
                               const std::string& hostDev,
                               const uint32_t& mode)
{
    Lock lock(mReconnectMutex);
    netdev::createMacvlan(mZone.getInitPid(), zoneDev, hostDev, static_cast<macvlan_mode>(mode));
}

void Zone::moveNetdev(const std::string& devId)
{
    Lock lock(mReconnectMutex);
    netdev::movePhys(mZone.getInitPid(), devId);
}

void Zone::destroyNetdev(const std::string& devId)
{
    Lock lock(mReconnectMutex);
    netdev::destroyNetdev(devId, mZone.getInitPid());
}

void Zone::goForeground()
{
    Lock lock(mReconnectMutex);
    setSchedulerLevel(SchedulerLevel::FOREGROUND);
}

void Zone::goBackground()
{
    Lock lock(mReconnectMutex);
    setSchedulerLevel(SchedulerLevel::BACKGROUND);
}

void Zone::setDetachOnExit()
{
    Lock lock(mReconnectMutex);
    mDetachOnExit = true;
}

void Zone::setDestroyOnExit()
{
    Lock lock(mReconnectMutex);
    mDestroyOnExit = true;
}

bool Zone::isRunning()
{
    Lock lock(mReconnectMutex);
    return mZone.getState() == lxc::LxcZone::State::RUNNING;
}

bool Zone::isStopped()
{
    Lock lock(mReconnectMutex);
    return mZone.getState() == lxc::LxcZone::State::STOPPED;
}

void Zone::suspend()
{
    Lock lock(mReconnectMutex);

    LOGD(mId << ": Pausing...");
    if (!mZone.freeze()) {
        throw ZoneOperationException("Could not pause zone");
    }
    LOGD(mId << ": Paused");

    updateRequestedState(STATE_PAUSED);
}

void Zone::resume()
{
    Lock lock(mReconnectMutex);

    LOGD(mId << ": Resuming...");
    if (!mZone.unfreeze()) {
        throw ZoneOperationException("Could not resume zone");
    }
    LOGD(mId << ": Resumed");

    updateRequestedState(STATE_RUNNING);
}

bool Zone::isPaused()
{
    Lock lock(mReconnectMutex);
    return mZone.getState() == lxc::LxcZone::State::FROZEN;
}

bool Zone::isSwitchToDefaultAfterTimeoutAllowed() const
{
    return mConfig.switchToDefaultAfterTimeout;
}

int Zone::createFile(const std::string& path, const std::int32_t flags, const std::int32_t mode)
{
    int fd = 0;
    if (!mZone.createFile(path, flags, mode, &fd)) {
        throw ZoneOperationException("Failed to create file.");
    }
    return fd;
}

std::string Zone::declareFile(const int32_t& type,
                              const std::string& path,
                              const int32_t& flags,
                              const int32_t& mode)
{
    return mProvision->declareFile(type, path, flags, mode);
}

std::string Zone::declareMount(const std::string& source,
                               const std::string& target,
                               const std::string& type,
                               const int64_t& flags,
                               const std::string& data)
{
    return mProvision->declareMount(source, target, type, flags, data);
}

std::string Zone::declareLink(const std::string& source,
                              const std::string& target)
{
    return mProvision->declareLink(source, target);
}

std::vector<std::string> Zone::getDeclarations() const
{
    return mProvision->list();
}

void Zone::removeDeclaration(const std::string& declarationId)
{
    mProvision->remove(declarationId);
}

void Zone::setNetdevAttrs(const std::string& netdev, const NetdevAttrs& attrs)
{
    Lock lock(mReconnectMutex);
    netdev::setAttrs(mZone.getInitPid(), netdev, attrs);
}

Zone::NetdevAttrs Zone::getNetdevAttrs(const std::string& netdev)
{
    Lock lock(mReconnectMutex);
    return netdev::getAttrs(mZone.getInitPid(), netdev);
}

std::vector<std::string> Zone::getNetdevList()
{
    Lock lock(mReconnectMutex);
    return netdev::listNetdev(mZone.getInitPid());
}

void Zone::deleteNetdevIpAddress(const std::string& netdev, const std::string& ip)
{
    Lock lock(mReconnectMutex);
    netdev::deleteIpAddress(mZone.getInitPid(), netdev, ip);
}

std::int64_t Zone::getSchedulerQuota()
{
    std::string ret;
    if (!lxc::getCgroup(mId, "cpu", "cpu.cfs_quota_us", ret)) {
        LOGE(mId << ": Error while getting the zone's scheduler quota param");
        throw ZoneOperationException("Could not get scheduler quota param");
    }
    return std::stoll(ret);
}

void Zone::setSchedulerLevel(SchedulerLevel sched)
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

void Zone::setSchedulerParams(std::uint64_t cpuShares,
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

} // namespace vasum
