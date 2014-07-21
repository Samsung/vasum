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
#include "exception.hpp"

#include "logger/logger.hpp"
#include "utils/exception.hpp"
#include "utils/fs.hpp"
#include "utils/callback-wrapper.hpp"
#include "utils/scoped-gerror.hpp"

#include <cassert>
#include <cstring>
#include <fcntl.h>
#include <functional>
#include <glib.h>
#include <linux/input.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <vector>
#include <functional>

using namespace security_containers::utils;
namespace fs = boost::filesystem;

namespace security_containers {

namespace {
const int MAX_TIME_WINDOW_SEC = 10;
const int KEY_PRESSED = 1;
const int DEVICE_NAME_LENGTH = 256;
const int MAX_NUMBER_OF_EVENTS = 10;
const std::string DEVICE_DIR = "/dev/input/";

} // namespace


InputMonitor::InputMonitor(const InputConfig& inputConfig,
                           const NotifyCallback& notifyCallback)
    : mConfig(inputConfig),
      mNotifyCallback(notifyCallback)
{
    if (mConfig.timeWindowMs > MAX_TIME_WINDOW_SEC * 1000L) {
        LOGE("Time window exceeds maximum: " << MAX_TIME_WINDOW_SEC);
        throw InputMonitorException();
    }

    if (mConfig.numberOfEvents > MAX_NUMBER_OF_EVENTS) {
        LOGE("Number of events exceeds maximum: " << MAX_NUMBER_OF_EVENTS);
        throw InputMonitorException();
    }

    std::string devicePath = getDevicePath();

    LOGT("Input monitor configuration: \n"
         << "\tenabled: " << mConfig.enabled << "\n"
         << "\tdevice: " << mConfig.device << "\n"
         << "\tpath: " << devicePath << "\n"
         << "\ttype: " << EV_KEY << "\n"
         << "\tcode: " << mConfig.code << "\n"
         << "\tvalue: " << KEY_PRESSED << "\n"
         << "\tnumberOfEvents: " << mConfig.numberOfEvents << "\n"
         << "\ttimeWindowMs: " << mConfig.timeWindowMs);

    createGIOChannel(devicePath);
}

InputMonitor::~InputMonitor()
{
    LOGD("Destroying InputMonitor");
    ScopedGError error;
    g_io_channel_unref(mChannelPtr);
    if (g_io_channel_shutdown(mChannelPtr, FALSE, &error)
            != G_IO_STATUS_NORMAL) {
        LOGE("Error during shutting down GIOChannel: " << error->message);
    }

    if (!g_source_remove(mSourceId)) {
        LOGE("Error during removing the source");
    }
}

namespace {
bool isDeviceWithName(const boost::regex& deviceNameRegex,
                      const fs::directory_entry& directoryEntry)
{
    std::string path = directoryEntry.path().string();

    if (!utils::isCharDevice(path)) {
        return false;
    }

    int fd = open(path.c_str(), O_RDONLY);
    if (fd < 0) {
        LOGD("Failed to open " << path);
        return false;
    }

    char name[DEVICE_NAME_LENGTH];
    if (ioctl(fd, EVIOCGNAME(sizeof(name)), name) < 0) {
        LOGD("Failed to get the device name of: " << path);
        if (close(fd) < 0) {
            LOGE("Error during closing file " << path);
        }
        return false;
    }

    if (close(fd) < 0) {
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

std::string InputMonitor::getDevicePath()
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
        throw InputMonitorException();
    }

    return it->path().string();
}



void InputMonitor::createGIOChannel(const std::string& devicePath)
{
    // We need NONBLOCK for FIFOs in the tests
    int fd = open(devicePath.c_str(), O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        LOGE("Cannot create input monitor channel. Device file: " <<
             devicePath << " doesn't exist");
        throw InputMonitorException();
    }

    mChannelPtr = g_io_channel_unix_new(fd);

    // Read binary data
    if (g_io_channel_set_encoding(mChannelPtr,
                                  NULL,
                                  NULL) != G_IO_STATUS_NORMAL) {
        LOGE("Cannot set encoding for input monitor channel ");
        throw InputMonitorException();
    }

    using namespace std::placeholders;
    ReadDeviceCallback callback = std::bind(&InputMonitor::readDevice, this, _1);
    // Add the callback
    mSourceId = g_io_add_watch_full(mChannelPtr,
                                    G_PRIORITY_DEFAULT,
                                    G_IO_IN,
                                    readDeviceCallback,
                                    utils::createCallbackWrapper(callback, mGuard.spawn()),
                                    &utils::deleteCallbackWrapper<ReadDeviceCallback>);
    if (!mSourceId) {
        LOGE("Cannot add watch on device input file");
        throw InputMonitorException();
    }
}

gboolean InputMonitor::readDeviceCallback(GIOChannel* gio,
        GIOCondition /*condition*/,
        gpointer data)
{
    const ReadDeviceCallback& callback = utils::getCallbackFromPointer<ReadDeviceCallback>(data);
    callback(gio);
    return TRUE;
}

void InputMonitor::readDevice(GIOChannel* gio)
{
    struct input_event ie;
    gsize nBytesReadTotal = 0;
    gsize nBytesRequested = sizeof(struct input_event);
    ScopedGError error;

    do {
        gsize nBytesRead = 0;
        GIOStatus readStatus = g_io_channel_read_chars(gio,
                               &reinterpret_cast<gchar*>(&ie)[nBytesReadTotal],
                               nBytesRequested,
                               &nBytesRead,
                               &error);

        if (readStatus == G_IO_STATUS_ERROR) {
            LOGE("Read from input monitor channel failed: " << error->message);
            return;
        }

        nBytesRequested -= nBytesRead;
        nBytesReadTotal += nBytesRead;
    } while (nBytesRequested > 0);


    if (isExpectedEventSequence(ie)) {
        LOGI("Input monitor detected pattern.");
        mNotifyCallback();
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
} // namespace security_containers
