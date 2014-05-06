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
    content.reserve(t.tellg());
    t.seekg(0, std::ios::beg);

    content.assign((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());

    return content;
}

bool removeFile(const std::string& path)
{
    if (::unlink(path.c_str()) != 0 && errno != ENOENT) {
        LOGD("Could not remove file '" << path << "': " << strerror(errno));
        return false;
    }
    return true;
}

bool isDirectory(const std::string& path)
{
    struct stat s;
    return ::stat(path.c_str(), &s) == 0 && S_IFDIR == (s.st_mode & S_IFMT);
}

bool createDirectory(const std::string& path, mode_t mode)
{
    if (::mkdir(path.c_str(), mode) == 0 && ::chmod(path.c_str(), mode) == 0) {
        return true;
    }
    LOGD("Could not create directory '" << path << "': " << strerror(errno));
    return false;
}

bool createDirectories(const std::string& path, mode_t mode)
{
    if (isDirectory(path)) {
        return true;
    }
    std::string parent = dirName(path);
    if (!parent.empty() && parent != path) {
        if (!createDirectories(parent, mode)) {
            return false;
        }
    }

    return createDirectory(path, mode);
}

bool mountTmpfs(const std::string& path)
{
    if (::mount("tmpfs", path.c_str(), "tmpfs", MS_NOSUID | MS_NODEV, "mode=755") != 0) {
        LOGD("Mount failed for '" << path << "': " << strerror(errno));
        return false;
    }
    return true;
}

bool umount(const std::string& path)
{
    if (::umount(path.c_str()) != 0) {
        LOGD("Umount failed for '" << path << "': " << strerror(errno));
        return false;
    }
    return true;
}


} // namespace utils
} // namespace security_containers
