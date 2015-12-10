/*
 *  Copyright (C) 2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License version 2.1 as published by the Free Software Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

/**
 * @file
 * @author  Krzysztof Dynowski (k.dynowski@samsumg.com)
 * @brief   Control-groups management
 */

#include "config.hpp"

#include "lxcpp/cgroups/subsystem.hpp"
#include "lxcpp/exception.hpp"

#include "utils/exception.hpp"
#include "utils/text.hpp"
#include "utils/fs.hpp"
#include "logger/logger.hpp"

#include <istream>
#include <sstream>
#include <iterator>
#include <algorithm>
#include <iostream>

namespace lxcpp {

Subsystem::Subsystem(const std::string& name) : mName(name)
{
    if (mName.empty()) {
        const std::string msg = "CGroup name is empty";
        LOGE(msg);
        throw CGroupException(msg);
    }

    //find mount point for a name
    std::ifstream fileStream("/proc/mounts");
    if (!fileStream.good()) {
        const std::string msg = "Failed to open /proc/mounts";
        LOGE(msg);
        throw CGroupException(msg);
    }

    std::string line;
    while (std::getline(fileStream, line).good()) {
        std::istringstream iss(line);
        auto it = std::istream_iterator<std::string>(iss);
        it++; //skip device name (fake for cgroup filesystem type)
        std::string path = *it++;       //mount point
        if (it->compare("cgroup") != 0) {    //filesystem type
            continue;
        }
        it++; //skip filesystem type
        if (it->find(mName) != std::string::npos) {
            mPath = std::move(path);
            break;
        }
    }
}

bool Subsystem::isAvailable() const
{
    if (mName.empty()) {
        const std::string msg = "CGroup name is empty";
        LOGE(msg);
        throw CGroupException(msg);
    }
    std::vector<std::string> av = availableSubsystems();
    return std::find(av.begin(), av.end(), mName) != av.end();
}

bool Subsystem::isAttached() const
{
    return !mPath.empty();
}

const std::string& Subsystem::getMountPoint() const
{
    if (!isAttached()) {
        const std::string msg = "CGroup '" + mName + "' is not attached";
        LOGE(msg);
        throw CGroupException(msg);
    }
    return mPath;
}

void Subsystem::attach(const std::string& path, const std::vector<std::string>& subs)
{
    if (path.empty()) {
        const std::string msg = "Trying attach to emtpy path";
        LOGE(msg);
        throw CGroupException(msg);
    }
    if (!utils::createDirs(path,0777)) {
         throw CGroupException("Can't create mount point: " + path + ", " + utils::getSystemErrorMessage());
    }
    if (!utils::mount("cgroup", path, "cgroup", 0, utils::join(subs,","))) {
         throw CGroupException("Can't mount cgroup: " + path + ", " + utils::getSystemErrorMessage());
    }
}

void Subsystem::detach(const std::string& path)
{
    if (!utils::umount(path)) {
         throw CGroupException("Can't umount cgroup: " + path + ", " + utils::getSystemErrorMessage());
    }
}

std::vector<std::string> Subsystem::availableSubsystems()
{
    std::ifstream fileStream("/proc/cgroups");
    if (!fileStream.good()) {
        const std::string msg = "Failed to open /proc/cgroups";
        LOGE(msg);
        throw CGroupException(msg);
    }

    std::vector<std::string> subs;
    std::string line;
    while (std::getline(fileStream, line).good()) {
        if (utils::beginsWith(line, "#")) {
            continue;
        }
        std::istringstream iss(line);
        auto it = std::istream_iterator<std::string>(iss);
        std::string n = *it++; //subsystem name
        subs.push_back(n);
    }
    return subs;
}

std::vector<std::string> Subsystem::getCGroups(pid_t pid)
{
    std::ifstream fileStream("/proc/" + std::to_string(pid) + "/cgroup");
    if (!fileStream.good()) {
        const std::string msg = "Failed to open /proc/<pid>/cgroup";
        LOGE(msg);
        throw CGroupException(msg);
    }

    std::vector<std::string> subs;
    std::string line;
    while (std::getline(fileStream, line).good()) {
        if (utils::beginsWith(line, "#")) {
            continue;
        }
        // istream_iterator does not support delimiter
        std::istringstream iss(line);
        std::string n, p;
        std::getline(iss, n, ':'); // ignore
        std::getline(iss, n, ':'); // subsystem name
        std::getline(iss, p, ':'); // cgroup path
        subs.push_back(n + ":" + p);
    }
    return subs;
}

} //namespace lxcpp
