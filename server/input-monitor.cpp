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
 * @brief   C++ wrapper for glib input monitor
 */

#include "config.hpp"

#include "input-monitor-config.hpp"
#include "input-monitor.hpp"
#include "zones-manager.hpp"
#include "exception.hpp"

#include "logger/logger.hpp"
#include "utils/exception.hpp"
#include "utils/fs.hpp"
#include "utils/fd-utils.hpp"

#include <cassert>
#include <cstring>
#include <fcntl.h>
#include <linux/input.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <vector>
#include <functional>

using namespace utils;
namespace fs = boost::filesystem;

namespace vasum {

namespace {
const int MAX_TIME_WINDOW_SEC = 10;
const int KEY_PRESSED = 1;
const int DEVICE_NAME_LENGTH = 256;
const int MAX_NUMBER_OF_EVENTS = 10;
const std::string DEVICE_DIR = "/dev/input/";

} // namespace


InputMonitor::InputMonitor(ipc::epoll::EventPoll& eventPoll,
                           const InputConfig& inputConfig,
                           ZonesManager* zonesManager)
    : mConfig(inputConfig)
    , mZonesManager(zonesManager)
    , mFd(-1)
    , mEventPoll(eventPoll)
{
    if (mConfig.timeWindowMs > MAX_TIME_WINDOW_SEC * 1000L) {
        const std::string msg = "Time window exceeds maximum: " + MAX_TIME_WINDOW_SEC;
        LOGE(msg);
        throw TimeoutException(msg);
    }

    if (mConfig.numberOfEvents > MAX_NUMBER_OF_EVENTS) {
        const std::string msg = "Number of events exceeds maximum: " + MAX_NUMBER_OF_EVENTS;
        LOGE(msg);
        throw InputMonitorException(msg);
    }

    mDevicePath = getDevicePath();

    LOGT("Input monitor configuration: \n"
         << "\tenabled: " << mConfig.enabled << "\n"
         << "\tdevice: " << mConfig.device << "\n"
         << "\tpath: " << mDevicePath << "\n"
         << "\ttype: " << EV_KEY << "\n"
         << "\tcode: " << mConfig.code << "\n"
         << "\tvalue: " << KEY_PRESSED << "\n"
         << "\tnumberOfEvents: " << mConfig.numberOfEvents << "\n"
         << "\ttimeWindowMs: " << mConfig.timeWindowMs);
}

InputMonitor::~InputMonitor()
{
    std::unique_lock<std::mutex> mMutex;
    LOGD("Destroying InputMonitor");
    stop();
}

void InputMonitor::start()
{
    std::unique_lock<Mutex> mMutex;
    setHandler(mDevicePath);
}

void InputMonitor::stop()
{
    leaveDevice();
}

namespace {

bool isDeviceWithName(const boost::regex& deviceNameRegex,
                      const fs::directory_entry& directoryEntry)
{
    std::string path = directoryEntry.path().string();

    if (!utils::isCharDevice(path)) {
        return false;
    }

    int fd = ::open(path.c_str(), O_RDONLY);
    if (fd < 0) {
        LOGD("Failed to open " << path);
        return false;
    }

    char name[DEVICE_NAME_LENGTH];
    if (::ioctl(fd, EVIOCGNAME(sizeof(name)), name) < 0) {
        LOGD("Failed to get the device name of: " << path);
        if (::close(fd) < 0) {
            LOGE("Error during closing file " << path);
        }
        return false;
    }

    if (::close(fd) < 0) {
        LOGE("Error during closing file " << path);
    }

    LOGD("Checking device: " << name);
    if (boost::regex_match(name, deviceNameRegex)) {
        LOGI("Device file found under: " << path);
        return true;
    }

    return false;
}

} // namespace

std::string InputMonitor::getDevicePath() const
{
    std::string device = mConfig.device;
    if (fs::path(device).is_absolute()
            && fs::exists(device)) {
        LOGD("Device file path is given");
        return device;
    }

    // device name is given - device file path is to be determined
    LOGT("Determining, which device node is assigned to '" << device << "'");
    using namespace std::placeholders;
    fs::directory_iterator end;
    boost::regex deviceNameRegex(".*" + device + ".*");
    const auto it = std::find_if(fs::directory_iterator(DEVICE_DIR),
                                 end,
                                 std::bind(isDeviceWithName, deviceNameRegex, _1));
    if (it == end) {
        LOGE("None of the files under '" << DEVICE_DIR <<
             "' represents device named: " << device);
        throw InputMonitorException("Cannot find a device");
    }

    return it->path().string();
}

void InputMonitor::setHandler(const std::string& devicePath)
{
    using namespace std::placeholders;

    // We need NONBLOCK for FIFOs in the tests
    mFd = ::open(devicePath.c_str(), O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    if (mFd < 0) {
        const std::string msg = "Cannot create input monitor channel. Device file: " +
                                devicePath + " doesn't exist";
        LOGE(msg);
        throw InputMonitorException(msg);
    }
    mEventPoll.addFD(mFd, EPOLLIN, std::bind(&InputMonitor::handleInternal, this, _1, _2));
}

void InputMonitor::handleInternal(int /* fd */, ipc::epoll::Events events)
{
    struct input_event ie;
    try {
        if (events == EPOLLHUP) {
            stop();
            return;
        }
        utils::read(mFd, &ie, sizeof(struct input_event));
    } catch (const std::exception& ex) {
        LOGE("Read from input monitor channel failed: " << ex.what());
        return;
    }
    if (isExpectedEventSequence(ie)) {
        LOGI("Input monitor detected pattern.");
        if (mZonesManager->isRunning()) {
            mZonesManager->switchingSequenceMonitorNotify();
        }
    }
}

void InputMonitor::leaveDevice()
{
    if (mFd != -1) {
        mEventPoll.removeFD(mFd);
        utils::close(mFd);
        mFd = -1;
    }
}

bool InputMonitor::isExpectedEventSequence(const struct input_event& ie)
{
    LOGT("Event detected [" << mConfig.device.c_str() << "]:\n"
         << "\ttime: " << ie.time.tv_sec << "." << ie.time.tv_usec << " sec\n"
         << "\ttype, code, value: " << ie.type << ", " << ie.code << ", " << ie.value);

    if (ie.type != EV_KEY
            || ie.code != mConfig.code
            || ie.value != KEY_PRESSED) {
        LOGT("Wrong kind of event");
        return false;
    }

    mEventTimes.push_back(ie.time);

    if (mEventTimes.size() < static_cast<unsigned int>(mConfig.numberOfEvents)) {
        LOGT("Event sequence too short");
        return false;
    }

    struct timeval oldest = mEventTimes.front();
    mEventTimes.pop_front();
    struct timeval latest = mEventTimes.back();

    long int secDiff = latest.tv_sec - oldest.tv_sec;
    if (secDiff >= MAX_TIME_WINDOW_SEC) {
        LOGT("Time window exceeded");
        return false;
    }

    long int timeDiff = secDiff * 1000L;
    timeDiff += static_cast<long int>((latest.tv_usec - oldest.tv_usec) / 1000L);
    if (timeDiff < mConfig.timeWindowMs) {
        LOGD("Event sequence detected");
        mEventTimes.clear();
        return true;
    }

    LOGT("Event sequence not detected");
    return false;
}
} // namespace vasum
