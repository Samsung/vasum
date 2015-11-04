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
 * @brief   Add new provisioned file/dir/link/mount command
 */

#include "lxcpp/commands/provision.hpp"
#include "lxcpp/container.hpp"
#include "lxcpp/provision-config.hpp"
#include "lxcpp/filesystem.hpp"
#include "utils/fs.hpp"
#include <boost/filesystem.hpp>
#include <sys/mount.h>

// TODO: Cleanup file/path handling

namespace lxcpp {

namespace fs = boost::filesystem;

void Provisions::execute()
{
    for(const auto & file : mConfig.mProvisions.files) {
        ProvisionFile(mConfig, file).execute();
    }

    for(const auto & mount : mConfig.mProvisions.mounts) {
        ProvisionMount(mConfig, mount).execute();
    }

    for(const auto & link : mConfig.mProvisions.links) {
        ProvisionLink(mConfig, link).execute();
    }
}
void Provisions::revert()
{
    for(const auto & file : mConfig.mProvisions.files) {
        ProvisionFile(mConfig, file).revert();
    }

    for(const auto & mount : mConfig.mProvisions.mounts) {
        ProvisionMount(mConfig, mount).revert();
    }

    for(const auto & link : mConfig.mProvisions.links) {
        ProvisionLink(mConfig, link).revert();
    }
}

ProvisionFile::ProvisionFile(ContainerConfig &config, const provision::File &file)
    : mConfig(config), mFile(file)
{
    utils::assertIsAbsolute(file.path);
}

void ProvisionFile::execute()
{
    using namespace provision;

    switch (mFile.type) {
    case File::Type::DIRECTORY:
        if (!utils::createDirs(mFile.path, mFile.mode)) {
            const std::string msg = "Can't create dir: " + mFile.path;
            LOGE(msg);
            throw ProvisionException(msg);
        }
        break;

    case File::Type::FIFO:
        if (!utils::createFifo(mFile.path, mFile.mode)) {
            const std::string msg = "Failed to make fifo: " + mFile.path;
            LOGE(msg);
            throw ProvisionException(msg);
        }
        break;

    case File::Type::REGULAR:
        if ((mFile.flags & O_CREAT)) {
            if (!utils::createFile(mFile.path, mFile.flags, mFile.mode)) {
                const std::string msg = "Failed to create file: " + mFile.path;
                LOGE(msg);
                throw ProvisionException(msg);
            }
        } else {
            if (!utils::copyFile(mFile.path, mFile.path)) {
                const std::string msg = "Failed to copy file: " + mFile.path;
                LOGE(msg);
                throw ProvisionException(msg);
            }
        }
        break;
    }
}
void ProvisionFile::revert()
{
    // TODO decision: should remove the file?
}


ProvisionMount::ProvisionMount(ContainerConfig &config, const provision::Mount &mount)
    : mConfig(config), mMount(mount)
{
    utils::assertIsAbsolute(mount.target);
}

void ProvisionMount::execute()
{
    lxcpp::mount(mMount.source, mMount.target, mMount.type, mMount.flags, mMount.data);
}
void ProvisionMount::revert()
{
    lxcpp::umount(mMount.target, MNT_DETACH);
}


ProvisionLink::ProvisionLink(ContainerConfig &config, const provision::Link &link)
    : mConfig(config), mLink(link)
{
    utils::assertIsAbsolute(link.target);
}

void ProvisionLink::execute()
{
    const std::string srcHostPath = fs::path(mLink.source).normalize().string();

    if (!utils::createLink(srcHostPath, mLink.target)) {
        const std::string msg = "Failed to create hard link: " +  mLink.source;
        LOGE(msg);
        throw ProvisionException(msg);
    }
}
void ProvisionLink::revert()
{
    // TODO decision: should remove the link?
}

} // namespace lxcpp
