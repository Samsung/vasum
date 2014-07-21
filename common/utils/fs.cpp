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

#include "config.hpp"
#include "logger/logger.hpp"
#include "utils/fs.hpp"
#include "utils/paths.hpp"
#include "utils/exception.hpp"

#include <boost/filesystem.hpp>
#include <dirent.h>
#include <fstream>
#include <streambuf>
#include <cstring>
#include <cerrno>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <unistd.h>


namespace security_containers {
namespace utils {


std::string readFileContent(const std::string& path)
{
    std::string result;
    if (!readFileContent(path, result)) {
        throw UtilsException();
    }
    return result;
}

bool readFileContent(const std::string& path, std::string& result)
{
    std::ifstream file(path);

    if (!file) {
        LOGD(path << ": could not open for reading");
        return false;
    }

    file.seekg(0, std::ios::end);
    std::streampos length = file.tellg();
    if (length < 0) {
        LOGD(path << ": tellg failed");
        return false;
    }
    result.resize(static_cast<size_t>(length));
    file.seekg(0, std::ios::beg);

    file.read(&result[0], length);
    if (!file) {
        LOGD(path << ": read error");
        result.clear();
        return false;
    }

    return true;
}

bool saveFileContent(const std::string& path, const std::string& content)
{
    std::ofstream file(path);
    if (!file) {
        LOGD(path << ": could not open for writing");
        return false;
    }
    file.write(content.data(), static_cast<std::streamsize>(content.size()));
    if (!file) {
        LOGD(path << ": could not write to");
        return false;
    }
    return true;
}

bool isCharDevice(const std::string& path)
{
    struct stat s;
    return ::stat(path.c_str(), &s) == 0 && S_IFCHR == (s.st_mode & S_IFMT);
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
    std::string parentPath = dirName(path);
    bool newResult;
    bool ret = hasSameMountPoint(path, parentPath, newResult);

    result = !newResult;
    return ret;
}

bool hasSameMountPoint(const std::string& path1, const std::string& path2, bool& result)
{
    struct stat s1, s2;

    if (::stat(path1.c_str(), &s1)) {
        LOGD("Failed to get stat of " << path1 << ": " << strerror(errno));
        return false;
    }

    if (::stat(path2.c_str(), &s2)) {
        LOGD("Failed to get stat of " << path2 << ": " << strerror(errno));
        return false;
    }

    result = (s1.st_dev == s2.st_dev);
    return true;
}

bool moveFile(const std::string& src, const std::string& dst)
{
    bool bResult;

    namespace fs = boost::filesystem;
    boost::system::error_code error;

    // The destination has to be a full path (including a file name)
    // so it doesn't exist yet, we need to check upper level dir instead.
    if (!hasSameMountPoint(src, dirName(dst), bResult)) {
        LOGE("Failed to check the files' mount points");
        return false;
    }

    if (bResult) {
        fs::rename(src, dst, error);
        if (error) {
            LOGE("Failed to rename the file: " << error);
            return false;
        }
    } else {
        fs::copy_file(src, dst, error);
        if (error) {
            LOGE("Failed to copy the file: " << error);
            return false;
        }
        fs::remove(src, error);
        if (error) {
            LOGE("Failed to remove the file: " << error);
            fs::remove(dst, error);
            return false;
        }
    }

    return true;
}

bool createDir(const std::string& path, uid_t uid, uid_t gid, boost::filesystem::perms mode)
{
    namespace fs = boost::filesystem;

    fs::path dirPath(path);
    boost::system::error_code ec;
    bool runDirCreated = false;
    if (!fs::exists(dirPath)) {
        if (!fs::create_directory(dirPath, ec)) {
            LOGE("Failed to create directory '" << path << "': "
                 << ec.message());
            return false;
        }
        runDirCreated = true;
    } else if (!fs::is_directory(dirPath)) {
        LOGE("Path '" << path << " already exists");
        return false;
    }

    // set permissions
    fs::permissions(dirPath, mode, ec);
    if (fs::status(dirPath).permissions() != mode) {
        LOGE("Failed to set permissions to '" << path << "': "
             << ec.message());
        return false;
    }

    // set owner
    if (::chown(path.c_str(), uid, gid) != 0) {
        // remove the directory only if it hadn't existed before
        if (runDirCreated) {
            fs::remove(dirPath);
        }
        LOGE("chown() failed for path '" << path << "': " << strerror(errno));
        return false;
    }

    return true;
}

} // namespace utils
} // namespace security_containers
