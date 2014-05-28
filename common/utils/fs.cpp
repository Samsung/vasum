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
 * @brief   File utility functions
 */

#include "log/logger.hpp"
#include "utils/fs.hpp"
#include "utils/paths.hpp"
#include "utils/exception.hpp"

#include <fstream>
#include <streambuf>
#include <cstring>
#include <cerrno>
#include <sys/stat.h>
#include <sys/mount.h>
#include <unistd.h>


namespace security_containers {
namespace utils {


std::string readFileContent(const std::string& path)
{
    std::ifstream t(path);

    if (!t.is_open()) {
        LOGE(path << " is missing");
        throw UtilsException();
    }

    std::string content;

    t.seekg(0, std::ios::end);
    content.reserve(static_cast<size_t>(t.tellg()));
    t.seekg(0, std::ios::beg);

    content.assign((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());

    return content;
}


namespace {
// NOTE: Should be the same as in systemd/src/core/mount-setup.c
const std::string RUN_MOUNT_POINT_OPTIONS = "mode=755,smackfstransmute=System::Run";
const std::string RUN_MOUNT_POINT_OPTIONS_NO_SMACK = "mode=755";
const unsigned long RUN_MOUNT_POINT_FLAGS = MS_NOSUID | MS_NODEV | MS_STRICTATIME;

bool mountTmpfs(const std::string& path, unsigned long flags, const std::string& options)
{
    if (::mount("tmpfs", path.c_str(), "tmpfs", flags, options.c_str()) != 0) {
        LOGD("Mount failed for '" << path << "', options=" << options << ": " << strerror(errno));
        return false;
    }
    return true;
}

} // namespace

bool mountRun(const std::string& path)
{
    return utils::mountTmpfs(path, RUN_MOUNT_POINT_FLAGS, RUN_MOUNT_POINT_OPTIONS)
           || utils::mountTmpfs(path, RUN_MOUNT_POINT_FLAGS, RUN_MOUNT_POINT_OPTIONS_NO_SMACK);
}

bool umount(const std::string& path)
{
    if (::umount(path.c_str()) != 0) {
        LOGD("Umount failed for '" << path << "': " << strerror(errno));
        return false;
    }
    return true;
}

bool isMountPoint(const std::string& path, bool& result)
{
    struct stat stat, parentStat;
    std::string parentPath = dirName(path);

    if (::stat(path.c_str(), &stat)) {
        LOGD("Failed to get stat of " << path << ": " << strerror(errno));
        return false;
    }

    if (::stat(parentPath.c_str(), &parentStat)) {
        LOGD("Failed to get stat of " << parentPath << ": " << strerror(errno));
        return false;
    }

    result = (stat.st_dev != parentStat.st_dev);
    return true;
}


} // namespace utils
} // namespace security_containers
