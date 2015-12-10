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
 * @author  Maciej Karpiuk (m.karpiuk2@samsung.com)
 * @brief   Provisioning configuration
 */

#include "config.hpp"

#include <vector>
#include <string>
#include "lxcpp/container-config.hpp"

using namespace lxcpp;
using namespace provision;

void ProvisionConfig::addFile(const File& newFile)
{
    auto it = std::find(files.begin(), files.end(), newFile);
    if (it != files.end()) {
        const std::string msg =
            "Can't add file. Provision already exists: " + newFile.getId();
        LOGE(msg);
        throw ProvisionException(msg);
    }
    files.push_back(newFile);
}

const FileVector& ProvisionConfig::getFiles() const
{
    return files;
}

void ProvisionConfig::removeFile(const File& item)
{
    const auto it = std::find(files.begin(), files.end(), item);
    if (it == files.end()) {
        const std::string msg = "Can't find provision: " + item.getId();
        LOGE(msg);
        throw ProvisionException(msg);
    }
    files.erase(it);
}


void ProvisionConfig::addMount(const Mount& newMount)
{
    auto it = std::find(mounts.begin(), mounts.end(), newMount);
    if (it != mounts.end()) {
        const std::string msg =
            "Can't add mount. Provision already exists: " + newMount.getId();
        LOGE(msg);
        throw ProvisionException(msg);
    }
    mounts.push_back(newMount);
}

const MountVector& ProvisionConfig::getMounts() const
{
    return mounts;
}

void ProvisionConfig::removeMount(const Mount& item)
{
    const auto it = std::find(mounts.begin(), mounts.end(), item);
    if (it == mounts.end()) {
        const std::string msg = "Can't find provision: " + item.getId();
        LOGE(msg);
        throw ProvisionException(msg);
    }
    mounts.erase(it);
}


void ProvisionConfig::addLink(const Link& newLink)
{
    auto it = std::find(links.begin(), links.end(), newLink);
    if (it != links.end()) {
        const std::string msg =
            "Can't add link. Provision already exists: " + newLink.getId();
        LOGE(msg);
        throw ProvisionException(msg);
    }
    links.push_back(newLink);
}

const LinkVector& ProvisionConfig::getLinks() const
{
    return links;
}

void ProvisionConfig::removeLink(const Link& item)
{
    const auto it = std::find(links.begin(), links.end(), item);
    if (it == links.end()) {
        const std::string msg = "Can't find provision: " + item.getId();
        LOGE(msg);
        throw ProvisionException(msg);
    }
    links.erase(it);
}
