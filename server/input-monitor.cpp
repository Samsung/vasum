/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Pawel Broda <p.broda@partner.samsung.com>
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
 * @author  Pawel Broda (p.broda@partner.samsung.com)
 * @brief   C++ wrapper for glib input monitor
 */

#include "exception.hpp"
#include "input-monitor.hpp"
#include "input-monitor-config.hpp"

#include "log/logger.hpp"
#include "utils/exception.hpp"
#include "utils/fs.hpp"

#include <cassert>
#include <cstring>
#include <fcntl.h>
#include <functional>
#include <glib.h>
#include <linux/input.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>


namespace security_containers {

// TODO: extract device helper and monitoring utilities from InputMonitor to separate files


const std::string InputMonitor::DEVICE_DIR = "/dev/input/";


std::string InputMonitor::findDeviceNode(const std::string& deviceName)
{
    std::vector<std::string> files;

    try {
        utils::listDir(DEVICE_DIR, files);
    } catch (UtilsException&) {
        throw InputMonitorException();
    }

    for (auto fileName: files) {
        std::string fullDevicePath = DEVICE_DIR + fileName;
        if (utils::isCharDevice(fullDevicePath)) {
            char name[DEVICE_NAME_LENGTH];
            memset(name, 0, sizeof(name));
            int fd = open(fullDevicePath.c_str(), O_RDONLY);
            if (fd < 0) {
                LOGD("Failed to open'" << fullDevicePath << "'");
                continue;
            }

            if (ioctl(fd, EVIOCGNAME(sizeof(name)), name) < 0) {
                LOGD("Failed to send ioctl with request about device name to '"
                     << fullDevicePath << "'");
                close(fd);
                continue;
            }

            close(fd);

            if (deviceName == name) {
                LOGI("Device file for '" << deviceName << "' found under '"
                     << fullDevicePath << "'");
                return fullDevicePath;
            }
        }
    }

    return std::string();
}

bool InputMonitor::detectExpectedEvent(const struct input_event& ie)
{
    // log events. Example log:
    // Event detected [/dev/input/event4]:
    //         time: 946705528.918966 sec
    //         type, code, value: 1, 139, 1
    LOGT("Event detected [" << mConfig.device.c_str() << "]:\n"
         << "\ttime: " << ie.time.tv_sec << "." << ie.time.tv_usec << " sec\n"
         << "\ttype, code, value: " << ie.type << ", " << ie.code << ", " << ie.value);

    if (ie.type == EV_KEY &&
        ie.code == mConfig.code &&
        ie.value == KEY_PRESSED) {
        for (int i = mConfig.numberOfEvents - 1; i > 0; --i) {
            mEventTime[i].tv_sec = mEventTime[i - 1].tv_sec;
            mEventTime[i].tv_usec = mEventTime[i - 1].tv_usec;
        }

        mEventTime[0].tv_sec = ie.time.tv_sec;
        mEventTime[0].tv_usec = ie.time.tv_usec;

        // a) check, if given event sequence happened within specified time window
        //    (this is necessary, because of multiplying in the next step)
        // b) if yes, then compute the difference between the first and the last one
        if (((mEventTime[0].tv_sec - mEventTime[mConfig.numberOfEvents - 1].tv_sec) < MAX_TIME_WINDOW_SEC) &&
            (mEventTime[0].tv_sec - mEventTime[mConfig.numberOfEvents - 1].tv_sec) * 1000000L +
             mEventTime[0].tv_usec - mEventTime[mConfig.numberOfEvents - 1].tv_usec < mConfig.timeWindowMs * 1000L) {

            resetEventsTime();

            return true;
        }
    }

    return false;
}

void InputMonitor::readDevice(GIOChannel* gio)
{
    struct input_event ie;
    gchar buff[sizeof(struct input_event)];
    gsize nBytesRequested = sizeof(struct input_event);
    gsize nBytesRead = 0;
    gsize nBytesReadTotal = 0;
    GError *err = NULL;

    do {
        GIOStatus readStatus = g_io_channel_read_chars(gio,
                                                       &buff[nBytesReadTotal],
                                                       nBytesRequested,
                                                       &nBytesRead,
                                                       &err);

         if (readStatus == G_IO_STATUS_ERROR || err != NULL) {
             LOGE("Read from input monitor channel failed");
             return;
         }

        nBytesRequested -= nBytesRead;
        nBytesReadTotal += nBytesRead;
    } while (nBytesRequested > 0);

    memcpy(&ie, buff, sizeof(struct input_event));

    if (detectExpectedEvent(ie)) {
        LOGI("Input monitor detected pattern.");
        mNotifyCallback();
    }
}

gboolean InputMonitor::readDeviceCallback(GIOChannel* gio,
                                          GIOCondition /*condition*/,
                                          gpointer data)
{
        InputMonitor& inputMonitor = *reinterpret_cast<InputMonitor*>(data);
        inputMonitor.readDevice(gio);

        return TRUE;
}

InputMonitor::InputMonitor(const InputConfig& inputConfig,
                           const InputNotifyCallback& inputNotifyCallback)
{
    mConfig = inputConfig;
    mNotifyCallback = inputNotifyCallback;

    resetEventsTime();

    if (mConfig.numberOfEvents > MAX_NUMBER_OF_EVENTS) {
        LOGE("Input monitor numberOfEvents > MAX_NUMBER_OF_EVENTS\n"
             << "\twhere MAX_NUMBER_OF_EVENTS = " << MAX_NUMBER_OF_EVENTS);
        throw InputMonitorException();
    }

    if (mConfig.timeWindowMs > MAX_TIME_WINDOW_SEC * 1000L) {
        LOGE("Input monitor timeWindowMs > MAX_TIME_WINDOW_SEC * 1000L\n"
             << "\twhere MAX_TIME_WINDOW_SEC = " << MAX_TIME_WINDOW_SEC);
        throw InputMonitorException();
    }

    std::string devicePath;
    if (mConfig.device[0] == '/') { // device file path is given
        devicePath = mConfig.device;
    } else { // device name is given - device file path is to be determined
        LOGT("Determining, which device node is assigned to '" << mConfig.device << "'");
        devicePath = findDeviceNode(mConfig.device);
        if (devicePath.empty()) {
            LOGE("None of the files under '" << DEVICE_DIR << "' represents device named: "
                 << mConfig.device.c_str());
                throw InputMonitorException();
        }
    }

    LOGT("Input monitor configuration: \n"
         << "\tenabled: " << mConfig.enabled << "\n"
         << "\tdevice: " << mConfig.device.c_str() << "\n"
         << "\tpath: " << devicePath.c_str() << "\n"
         << "\ttype: " << EV_KEY << "\n"
         << "\tcode: " << mConfig.code << "\n"
         << "\tvalue: " << KEY_PRESSED << "\n"
         << "\tnumberOfEvents: " << mConfig.numberOfEvents << "\n"
         << "\ttimeWindowMs: " << mConfig.timeWindowMs);

    GError *err = NULL;

    // creating channel using steps below, i.e.:
    // 1) open()
    // 2) g_io_channel_unix_new()
    // was chosen, because glib API does not allow to open a file
    // in a non-blocking mode. There is no argument to pass additional
    // flags and we need such a mode for FIFOs in the tests.
    int fd = open(devicePath.c_str(), O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        LOGE("Cannot create input monitor channel. Device file: " << devicePath.c_str()
             << " doesn't exist" );
        throw InputMonitorException();
    }

    GIOChannel *mChannel = g_io_channel_unix_new(fd);

    // close the channel on final unref
    g_io_channel_set_close_on_unref(mChannel,
                                    TRUE);

    // read binary
    g_io_channel_set_encoding(mChannel,
                              NULL,
                              &err);

    if (err != NULL) {
        LOGE("Cannot set encoding for input monitor channel");
        throw InputMonitorException();
    }

    if (!g_io_add_watch(mChannel, G_IO_IN, readDeviceCallback, this)) {
        LOGE("Cannot add watch on device input file");
        throw InputMonitorException();
    }
}

void InputMonitor::resetEventsTime()
{
    memset(mEventTime, 0, sizeof(struct timeval) * MAX_NUMBER_OF_EVENTS);
}

InputMonitor::~InputMonitor()
{
}

} // namespace security_containers
