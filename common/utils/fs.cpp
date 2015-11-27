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
#include <fcntl.h>

#include <iostream>


namespace fs = boost::filesystem;

namespace utils {

std::string readFileStream(const std::string& path)
{
    std::ifstream file(path);

    if (!file) {
        throw UtilsException("Read failed");
    }
    // 2 x faster then std::istreambuf_iterator
    std::stringstream content;
    content << file.rdbuf();
    return content.str();
}

bool readFileStream(const std::string& path, std::string& result)
{
    std::ifstream file(path);

    if (!file) {
        return false;
    }
    std::stringstream content;
    content << file.rdbuf();
    result = content.str();
    return true;
}

std::string readFileContent(const std::string& path)
{
    std::string result;

    if (!readFileContent(path, result)) {
        throw UtilsException("Read failed");
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

bool readFirstLineOfFile(const std::string& path, std::string& ret)
{
    std::ifstream file(path);
    if (!file) {
        LOGD(path << ": could not open for reading");
        return false;
    }

    std::getline(file, ret);
    if (!file) {
        LOGD(path << ": read error");
        return false;
    }
    return true;
}

bool removeFile(const std::string& path)
{
    if (::remove(path.c_str())) {
        if (errno != ENOENT) {
            LOGE(path << ": failed to delete: " << getSystemErrorMessage());
            return false;
        }
    }
    LOGD(path << ": successfuly removed.");

    return true;
}

bool exists(const std::string& path, int inodeType)
{
    try {
        assertExists(path, inodeType);
        return true;
    } catch(...) {
        return false;
    }
}

void assertExists(const std::string& path, int inodeType)
{
    if (path.empty()) {
        const std::string msg = "Empty path";
        LOGE(msg);
        throw UtilsException(msg);
    }

    struct stat s;
    if (::stat(path.c_str(), &s)) {
        const std::string msg = "Error in stat() " + path + ": " + getSystemErrorMessage();
        LOGE(msg);
        throw UtilsException(msg);
    }

    if (inodeType != 0) {
        if (!(s.st_mode & inodeType)) {
            const std::string msg = "Not an expected inode type, expected: " + std::to_string(inodeType) +
                                    ", while actual: " + std::to_string(s.st_mode);
            LOGE(msg);
            throw UtilsException(msg);
        }

        if (inodeType == S_IFDIR && ::access(path.c_str(), X_OK) < 0) {
            const std::string msg = "Not a traversable directory";
            LOGE(msg);
            throw UtilsException(msg);
        }
    }
}

bool isCharDevice(const std::string& path)
{
    return utils::exists(path, S_IFCHR);
}

bool isRegularFile(const std::string& path)
{
    return utils::exists(path, S_IFREG);
}

void assertIsRegularFile(const std::string& path)
{
    assertExists(path, S_IFREG);
}

bool isDir(const std::string& path)
{
    return utils::exists(path, S_IFDIR);
}

void assertIsDir(const std::string& path)
{
    assertExists(path, S_IFDIR);
}

bool isAbsolute(const std::string& path)
{
    return fs::path(path).is_absolute();
}

void assertIsAbsolute(const std::string& path)
{
    if (!isAbsolute(path)) {
        const std::string msg = "Given path '" + path + "' must be absolute!";
        LOGE(msg);
        throw UtilsException(msg);
    }
}


namespace {
// NOTE: Should be the same as in systemd/src/core/mount-setup.c
const std::string RUN_MOUNT_POINT_OPTIONS = "mode=755,smackfstransmute=System::Run";
const std::string RUN_MOUNT_POINT_OPTIONS_NO_SMACK = "mode=755";
const unsigned long RUN_MOUNT_POINT_FLAGS = MS_NOSUID | MS_NODEV | MS_STRICTATIME;

bool mountTmpfs(const std::string& path, unsigned long flags, const std::string& options)
{
    if (::mount("tmpfs", path.c_str(), "tmpfs", flags, options.c_str()) != 0) {
        LOGD("Mount failed for '" << path << "', options=" << options << ": " << getSystemErrorMessage());
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

bool mount(const std::string& source,
           const std::string& target,
           const std::string& filesystemtype,
           unsigned long mountflags,
           const std::string& data)
{
    int ret = ::mount(source.c_str(),
                      target.c_str(),
                      filesystemtype.c_str(),
                      mountflags,
                      data.c_str());
    if (ret < 0) {
        LOGE("Mount operation failure: "
             << "source path: "
             << source
             << ", target path: "
             << target
             << ", filesystemtype: "
             << filesystemtype
             << ", mountflags: "
             << mountflags
             << ", data: "
             << data
             << ", msg: "
             << getSystemErrorMessage());
        return false;
    }
    return true;
}

bool umount(const std::string& path)
{
    if (::umount(path.c_str()) != 0) {
        LOGE("Umount failed for '" << path << "': " << getSystemErrorMessage());
        return false;
    }
    return true;
}

bool isMountPoint(const std::string& path, bool& result)
{
    std::string parentPath = dirName(path);
    bool newResult;
    if (!hasSameMountPoint(path, parentPath, newResult)) {
        LOGE("Failed to check the files' mount points");
        return false;
    }

    result = !newResult;
    return true;
}

bool hasSameMountPoint(const std::string& path1, const std::string& path2, bool& result)
{
    struct stat s1, s2;

    if (::stat(path1.c_str(), &s1)) {
        LOGD("Failed to get stat of " << path1 << ": " << getSystemErrorMessage());
        return false;
    }

    if (::stat(path2.c_str(), &s2)) {
        LOGD("Failed to get stat of " << path2 << ": " << getSystemErrorMessage());
        return false;
    }

    result = (s1.st_dev == s2.st_dev);
    return true;
}

bool moveFile(const std::string& src, const std::string& dst)
{
    bool bResult;

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

namespace {

bool copyDirContentsRec(const boost::filesystem::path& src, const boost::filesystem::path& dst)
{
    try {
        for (fs::directory_iterator file(src);
                file != fs::directory_iterator();
                ++file) {
            fs::path current(file->path());
            fs::path destination = dst / current.filename();

            boost::system::error_code ec;

            if (!fs::is_symlink(current) && fs::is_directory(current)) {
                fs::create_directory(destination, ec);
            } else {
                fs::copy(current, destination, ec);
            }

            if (ec.value() != boost::system::errc::success) {
                LOGW("Failed to copy " << current << ": " << ec.message());
                continue;
            }

            if (!fs::is_symlink(current) && fs::is_directory(current)) {
                if (!copyDirContentsRec(current, destination)) {
                    return false;
                }

                // apply permissions coming from source file/directory
                fs::file_status stat = status(current);
                fs::permissions(destination, stat.permissions(), ec);

                if (ec.value() != boost::system::errc::success) {
                    LOGW("Failed to set permissions for " << destination << ": " << ec.message());
                }
            }

            // change owner
            struct stat info;
            ::stat(current.string().c_str(), &info);
            if (fs::is_symlink(destination)) {
                if (::lchown(destination.string().c_str(), info.st_uid, info.st_gid) < 0) {
                    LOGW("Failed to change owner of symlink " << destination.string() << ": " << getSystemErrorMessage());
                }
            } else {
                if (::chown(destination.string().c_str(), info.st_uid, info.st_gid) < 0) {
                    LOGW("Failed to change owner of file " << destination.string() << ": " << getSystemErrorMessage());
                }
            }
        }
    } catch (fs::filesystem_error& e) {
        LOGW(e.what());
    }

    return true;
}

boost::filesystem::perms getPerms(const mode_t& mode)
{
    return static_cast<boost::filesystem::perms>(mode);
}

bool copySmackLabel(const std::string& /* src */, const std::string& /* dst */)
{
    //TODO: fill copySmackLabel function
    return true;
}


} // namespace

bool copyDirContents(const std::string& src, const std::string& dst)
{
    return copyDirContentsRec(fs::path(src), fs::path(dst));
}

bool createDir(const std::string& path, uid_t uid, uid_t gid, boost::filesystem::perms mode)
{
    fs::path dirPath(path);
    boost::system::error_code errorCode;
    bool runDirCreated = false;
    if (!fs::exists(dirPath)) {
        if (!fs::create_directory(dirPath, errorCode)) {
            LOGE("Failed to create directory '" << path << "': "
                 << errorCode.message());
            return false;
        }
        runDirCreated = true;
    } else if (!fs::is_directory(dirPath)) {
        LOGE("Path '" << path << " already exists");
        return false;
    }

    // set permissions
    fs::permissions(dirPath, mode, errorCode);
    if (fs::status(dirPath).permissions() != mode) {
        LOGE("Failed to set permissions to '" << path << "': "
             << errorCode.message());
        return false;
    }

    // set owner
    if (::chown(path.c_str(), uid, gid) != 0) {
        int err = errno;
        // remove the directory only if it hadn't existed before
        if (runDirCreated) {
            fs::remove(dirPath);
        }
        LOGE("chown() failed for path '" << path << "': " << getSystemErrorMessage(err));
        return false;
    }

    return true;
}

bool createDirs(const std::string& path, mode_t mode)
{
    const boost::filesystem::perms perms = getPerms(mode);
    std::vector<fs::path> dirsCreated;
    fs::path prefix;
    const fs::path dirPath = fs::path(path);
    for (const auto& dirSegment : dirPath) {
        prefix /= dirSegment;
        if (!fs::exists(prefix)) {
            bool created = createDir(prefix.string(), -1, -1, perms);
            if (created) {
                dirsCreated.push_back(prefix);
            } else {
                LOGE("Failed to create dir");
                // undo
                for (auto iter = dirsCreated.rbegin(); iter != dirsCreated.rend(); ++iter) {
                    boost::system::error_code errorCode;
                    fs::remove(*iter, errorCode);
                    if (errorCode) {
                        LOGE("Error during cleaning: dir: " << *iter
                             << ", msg: " << errorCode.message());
                    }
                }
                return false;
            }
        }
    }
    return true;
}

bool createEmptyDir(const std::string& path)
{
    fs::path dirPath(path);
    boost::system::error_code ec;
    bool cleanDirCreated = false;

    if (!fs::exists(dirPath)) {
        if (!fs::create_directory(dirPath, ec)) {
            LOGE("Failed to create dir. Error: " << ec.message());
            return false;
        }
        cleanDirCreated = true;
    } else if (!fs::is_directory(dirPath)) {
        LOGE("Provided path already exists and is not a dir, cannot create.");
        return false;
    }

    if (!cleanDirCreated) {
        // check if directory is empty if it was already created
        if (!fs::is_empty(dirPath)) {
            LOGE("Directory has some data inside, cannot be used.");
            return false;
        }
    }

    return true;
}

bool createFile(const std::string& path, int flags, mode_t mode)
{
    int ret = ::open(path.c_str(), flags, mode);
    if (ret < 0) {
        LOGE("Failed to create file: path=host:"
             << path
             << ", msg: "
             << getSystemErrorMessage());
        return false;
    }
    close(ret);
    return true;
}

bool createFifo(const std::string& path, mode_t mode)
{
    int ret = ::mkfifo(path.c_str(), mode);
    if (ret < 0) {
        LOGE("Failed to make fifo: path=host:" << path);
        return false;
    }
    return true;
}

bool copyFile(const std::string& src, const std::string& dest)
{
    boost::system::error_code errorCode;
    fs::copy_file(src, dest, errorCode);
    if (errorCode) {
        LOGE("Failed to copy file: msg: "
             << errorCode.message()
             << ", path=host:"
             << src
             << ", path=host:"
             << dest);
        return false;
    }
    bool retSmack = copySmackLabel(src, dest);
    if (!retSmack) {
        LOGE("Failed to copy file: msg: (can't copy smacklabel) "
             << ", path=host:"
             << src
             << ", path=host:"
             << dest);
        fs::remove(src, errorCode);
        if (errorCode) {
            LOGE("Failed to clean after copy failure: path=host:"
                 << src
                 << ", msg: "
                 << errorCode.message());
        }
        return false;
    }
    return true;
}

bool createLink(const std::string& src, const std::string& dest)
{
    int retLink = ::link(src.c_str(), dest.c_str());
    if (retLink < 0) {
        LOGE("Failed to hard link: path=host:"
             << src
             << ", path=host:"
             << dest
             << ", msg:"
             << getSystemErrorMessage());
        return false;
    }
    bool retSmack = copySmackLabel(src, dest);
    if (!retSmack) {
        LOGE("Failed to copy smack label: path=host:"
             << src
             << ", path=host:"
             << dest);
        boost::system::error_code ec;
        fs::remove(dest, ec);
        if (!ec) {
            LOGE("Failed to clean after hard link creation failure: path=host:"
                 << src
                 << ", to: "
                 << dest
                 << ", msg: "
                 << ec.message());
        }
        return false;
    }
    return true;
}

} // namespace utils
