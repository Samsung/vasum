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

#include "lxcpp/cgroups/cgroup.hpp"
#include "lxcpp/exception.hpp"
#include "utils/fs.hpp"
#include "utils/text.hpp"
#include "logger/logger.hpp"

namespace fs = boost::filesystem;

namespace lxcpp {

namespace {
std::string getSubsysName(const std::string& s)
{
    auto p = s.find(':');
    if (p == std::string::npos) {
        const std::string msg = "wrong subsys format " + s;
        LOGE(msg);
        throw CGroupException(msg);
    }
    return s.substr(0, p);
}

std::string getCGroupName(const std::string& s)
{
    auto p = s.find(':');
    if (p == std::string::npos) {
        const std::string msg = "wrong cgroup format " + s;
        LOGE(msg);
        throw CGroupException(msg);
    }
    return s.substr(p + 1);
}

} // namespace

CGroup::CGroup(const std::string& subsysAndCgroup) :
    mSubsys(std::move(getSubsysName(subsysAndCgroup))),
    mName(std::move(getCGroupName(subsysAndCgroup)))
{
}

bool CGroup::exists() const
{
    const fs::path path = fs::path(mSubsys.getMountPoint()) / mName;
    return fs::is_directory(path);
}

void CGroup::create()
{
    const fs::path path = fs::path(mSubsys.getMountPoint()) / mName;
    fs::create_directory(path);
}

void CGroup::destroy()
{
    const fs::path path = fs::path(mSubsys.getMountPoint()) / mName;
    //remove_all is not good for cgroup filesystem
    fs::remove(path);
}

void CGroup::setCommonValue(const std::string& param, const std::string& value)
{
    const fs::path path = fs::path(mSubsys.getMountPoint()) / mName / ("cgroup." + param);
    if (!utils::saveFileContent(path.string(), value)) {
        const std::string msg = "Invalid param " + param;
        LOGE(msg);
        throw CGroupException(msg);
    }
}

std::string CGroup::getCommonValue(const std::string& param) const
{
    const fs::path path = fs::path(mSubsys.getMountPoint()) / mName / ("cgroup." + param);
    return utils::readFileStream(path.string());
}

void CGroup::setValue(const std::string& param, const std::string& value)
{
    const fs::path path = fs::path(mSubsys.getMountPoint()) / mName / (mSubsys.getName() + "." + param);
    if (!utils::saveFileContent(path.string(), value)) {
        const std::string msg = "Invalid param " + param;
        LOGE(msg);
        throw CGroupException(msg);
    }
}

std::string CGroup::getValue(const std::string& param) const
{
    const fs::path path = fs::path(mSubsys.getMountPoint()) / mName / (mSubsys.getName() + "." + param);
    return utils::readFileStream(path.string());
}

void CGroup::assignGroup(pid_t pid)
{
    setCommonValue("procs", std::to_string(pid));
}

void CGroup::assignPid(pid_t pid)
{
    const fs::path path = fs::path(mSubsys.getMountPoint()) / mName / "tasks";
    utils::saveFileContent(path.string(),  std::to_string(pid));
}

std::vector<pid_t> CGroup::getPids() const
{
    const fs::path path = fs::path(mSubsys.getMountPoint()) / mName / "tasks";
    std::ifstream fileStream(path.string());
    if (!fileStream.good()) {
        const std::string msg = "Failed to open " + path.string();
        LOGE(msg);
        throw CGroupException(msg);
    }

    std::vector<pid_t> pids;
    while (fileStream.good()) {
        int pid;
        fileStream >> pid;
        pids.push_back(pid);
    }

    return pids;
}

CGroup CGroup::getCGroup(const std::string& subsys, pid_t pid)
{
    std::vector<std::string> cgroups = Subsystem::getCGroups(pid);
    for (const auto& i : cgroups) {
        if (utils::beginsWith(i, subsys + ":")) {
            return CGroup(i);
        }
    }

    const std::string msg = "cgroup not found for pid " + std::to_string(pid);
    LOGE(msg);
    throw CGroupException(msg);
}

} //namespace lxcpp
