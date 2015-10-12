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
#include "logger/logger.hpp"

namespace fs = boost::filesystem;

namespace lxcpp {

namespace {
std::string getSubsysName(const std::string& s) {
    auto p = s.find(':');
    if (p == std::string::npos) {
        const std::string msg = "wgrong cgroup format";
        LOGE(msg);
        throw CGroupException(msg);
    }
    return s.substr(0, p);
}
std::string getCGroupName(const std::string& s) {
    auto p = s.find(':');
    if (p == std::string::npos) {
        const std::string msg = "wgrong cgroup format";
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
    fs::remove_all(path);
}

void CGroup::setValue(const std::string& param, const std::string& value)
{
    const fs::path path = fs::path(mSubsys.getMountPoint()) / mName / (mSubsys.getName() + "." + param);
    utils::saveFileContent(path.string(), value);
}

std::string CGroup::getValue(const std::string& param) const
{
    const fs::path path = fs::path(mSubsys.getMountPoint()) / mName / (mSubsys.getName() + "." + param);
    return utils::readFileContent(path.string());
}

void CGroup::assignProcess(pid_t pid)
{
    const fs::path path = fs::path(mSubsys.getMountPoint()) / mName / "tasks";
    utils::saveFileContent(path.string(),  std::to_string(pid));
}

} //namespace lxcpp
