/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Lukasz Kostyra <l.kostyra@samsung.com>
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
 * @author  Lukasz Kostyra (l.kostyra@samsung.com)
 * @brief   Image utility functions declaration
 */

#include "config.hpp"

#include "logger/logger.hpp"
#include "utils/img.hpp"
#include "utils/fs.hpp"
#include "utils/paths.hpp"
#include "utils/exception.hpp"

#include <sys/mount.h>
#include <fcntl.h>
#include <linux/loop.h>

namespace utils {

namespace {

const std::string LOOP_DEV_PREFIX = "/dev/loop";
const std::string LOOP_MOUNT_POINT_OPTIONS = "";
const std::string LOOP_MOUNT_POINT_TYPE = "ext4";
const unsigned long LOOP_MOUNT_POINT_FLAGS = MS_RDONLY;

// Writes to ret if loop device (provided in loopdev arg) is free to use.
// Returns true if check was successful, false if loop device FD was unavailable for some reason.
bool isLoopDevFree(const std::string& loopdev, bool& ret)
{
    // initialize
    ret = false;

    // open loop device FD
    int loopFD = ::open(loopdev.c_str(), O_RDWR);
    if (loopFD < 0) {
        LOGD("Failed to open loop device descriptor: " << getSystemErrorMessage());
        return false;
    }

    // if ioctl with LOOP_GET_STATUS fails, device is not assigned and free to use
    struct loop_info linfo;
    if (::ioctl(loopFD, LOOP_GET_STATUS, &linfo)) {
        ret = true;
    }

    ::close(loopFD);
    return true;
}

bool mountLoop(const std::string& img,
               const std::string& loopdev,
               const std::string& path,
               const std::string& type,
               unsigned long flags,
               const std::string& options)
{
    // to mount an image, we need to connect image FD with loop device FD
    // get image file  FD
    int fileFD = ::open(img.c_str(), O_RDWR);
    if (fileFD < 0) {
        LOGD("Failed to open image file descriptor: " << getSystemErrorMessage());
        return false;
    }

    // get loop device FD
    int loopFD = ::open(loopdev.c_str(), O_RDWR);
    if (loopFD < 0) {
        LOGD("Failed to open loop device descriptor: " << getSystemErrorMessage());
        ::close(fileFD);
        return false;
    }

    // set loop device
    if (::ioctl(loopFD, LOOP_SET_FD, fileFD)) {
        LOGD("Failed to assign loop device to image: " << getSystemErrorMessage());
        ::close(fileFD);
        ::close(loopFD);
        return false;
    }

    // mount loop device to path
    if (::mount(loopdev.c_str(), path.c_str(), type.c_str(), flags, options.c_str()) != 0) {
        LOGD("Mount failed for '" << path << "', options=" << options << ": " << getSystemErrorMessage());
        ::ioctl(loopFD, LOOP_CLR_FD, 0);
        ::close(fileFD);
        ::close(loopFD);
        return false;
    }

    ::close(fileFD);
    ::close(loopFD);
    return true;
}

} // namespace

// Finds first available loop device and returns its path through ret.
// Returns false if an error occurs, or if all available loop devices are taken.
bool getFreeLoopDevice(std::string& ret)
{
    for (unsigned int i = 0; i < 8; ++i) {
        // build path to loop device
        const std::string loopdev = LOOP_DEV_PREFIX + std::to_string(i);
        bool isFree = false;

        // check if it is free
        if (!isLoopDevFree(loopdev, isFree)) {
            LOGD("Failed to check status of " << loopdev);
            return false;
        }

        // if checked loop device is free, we can exit the function and return it
        if (isFree) {
            ret = loopdev;
            return true;
        }
    }

    LOGD("All loop devices are taken.");
    return false;
}

bool mountImage(const std::string& image, const std::string& path, const std::string& loopdev)
{
    return mountLoop(image, path, loopdev,
                     LOOP_MOUNT_POINT_TYPE,
                     LOOP_MOUNT_POINT_FLAGS,
                     LOOP_MOUNT_POINT_OPTIONS);
}

bool umountImage(const std::string& path, const std::string& loopdev)
{
    if (::umount(path.c_str()) != 0) {
        LOGD("Umount failed for '" << path << "': " << getSystemErrorMessage());
        return false;
    }

    // clear loop device
    int loopFD = ::open(loopdev.c_str(), O_RDWR);
    if (loopFD < 0) {
        LOGD("Failed to open fd for loop device 0");
        return false;
    }

    if (::ioctl(loopFD, LOOP_CLR_FD, 0) < 0) {
        LOGD("Failed to clear loop device.");
        close(loopFD);
        return false;
    }

    close(loopFD);
    return true;
}

bool copyImageContents(const std::string& img, const std::string& dst)
{
    namespace fs = boost::filesystem;
    boost::system::error_code ec;

    // make sure that image exists
    if (!fs::exists(fs::path(img))) {
        LOGE("Image " << img << " does not exist");
        return false;
    }

    const std::string mountPoint = createFilePath(dirName(img), "/mp/");
    // create a mount point for copied image
    if (!createEmptyDir(mountPoint)) {
        LOGE("Cannot create mount point for copied image.");
        return false;
    }

    // create dst directory
    if (!createEmptyDir(dst)) {
        LOGE("Cannot create directory for data.");
        return false;
    }

    // find free loop device for image
    std::string loopdev;
    if (!utils::getFreeLoopDevice(loopdev)) {
        LOGE("Failed to get free loop device.");
        return false;
    }

    LOGT("Using " << loopdev << " to mount image");
    // mount an image
    if (!utils::mountImage(img, loopdev, mountPoint)) {
        LOGE("Cannot mount image.");
        return false;
    }

    // copy data
    LOGI("Beginning image copy");
    if (!utils::copyDirContents(mountPoint, dst)) {
        LOGE("Failed to copy image.");
        utils::umountImage(mountPoint, loopdev);
        LOGD("Removing already copied data");
        fs::remove_all(fs::path(dst));
        return false;
    }
    LOGI("Finished image copy");

    // umount image
    if (!utils::umountImage(mountPoint, loopdev)) {
        LOGE("Failed to umount image");
        LOGD("Removing copied data");
        fs::remove_all(fs::path(dst));
        return false;
    }

    // remove mount point
    if (!fs::remove(fs::path(mountPoint), ec)) {
        LOGW("Failed to remove mount point: " << ec.message());
    }

    return true;
}

} // namespace utils
