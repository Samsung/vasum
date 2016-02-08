/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Piotr Bartosiewicz <p.bartosiewi@partner.samsung.com>
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
 * @author  Piotr Bartosiewicz (p.bartosiewi@partner.samsung.com)
 * @brief   Lxc cgroup stuff
 */

#include "config.hpp"

#include "lxc/cgroup.hpp"
#include "logger/logger.hpp"
#include "utils/fs.hpp"
#include "utils/paths.hpp"
#include "utils/exception.hpp"

#include <fcntl.h>


namespace vasum {
namespace lxc {

namespace {

std::string flagsToPermissions(bool grant, uint32_t flags)
{
    if (!grant) {
        return "rwm";
    }
    switch (flags) {
    case O_RDWR:
        return "rwm";
    case O_RDONLY:
        return "rm";
    case O_WRONLY:
        return "wm";
    default:
        return std::string();
    }
}

std::string getCgroupPath(const std::string& zoneName,
                          const std::string& cgroupController,
                          const std::string& cgroupName)
{
    return utils::createFilePath("/sys/fs/cgroup",
                                 cgroupController,
                                 "lxc",
                                 zoneName,
                                 cgroupName);
}

} // namespace

void setCgroup(const std::string& zoneName,
               const std::string& cgroupController,
               const std::string& cgroupName,
               const std::string& value)
{
    const std::string path = getCgroupPath(zoneName, cgroupController, cgroupName);
    LOGD("Set '" << value << "' to " << path);
    utils::saveFileContent(path, value);
}

std::string getCgroup(const std::string& zoneName,
                      const std::string& cgroupController,
                      const std::string& cgroupName)
{
    std::string retval;
    const std::string path = getCgroupPath(zoneName, cgroupController, cgroupName);
    utils::readFirstLineOfFile(path, retval);
    return retval;
}

bool isDevice(const std::string& device)
{
    struct stat devStat = utils::stat(device);
    if (!S_ISCHR(devStat.st_mode) && !S_ISBLK(devStat.st_mode)) {
        LOGD("Not a device: " << device);
        return false;
    }
    return true;
}

bool setDeviceAccess(const std::string& zoneName,
                     const std::string& device,
                     bool grant,
                     uint32_t flags)
{
    struct stat devStat = utils::stat(device);

    char type;
    if (S_ISCHR(devStat.st_mode)) {
        type = 'c';
    } else if (S_ISBLK(devStat.st_mode)) {
        type = 'b';
    } else {
        LOGD("Not a device: " << device);
        return false;
    }

    std::string perm = flagsToPermissions(grant, flags);
    if (perm.empty()) {
        LOGD("Invalid flags");
        return false;
    }

    unsigned int major = major(devStat.st_rdev);
    unsigned int minor = minor(devStat.st_rdev);

    char value[100];
    snprintf(value, sizeof(value), "%c %u:%u %s", type, major, minor, perm.c_str());

    std::string name = grant ? "devices.allow" : "devices.deny";
    setCgroup(zoneName, "devices", name, value);
    return true;
}


} // namespace lxc
} // namespace vasum
