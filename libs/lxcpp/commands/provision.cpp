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

#include "config.hpp"

#include "lxcpp/commands/provision.hpp"
#include "lxcpp/container.hpp"
#include "lxcpp/provision-config.hpp"
#include "utils/fs.hpp"
#include <boost/filesystem.hpp>
#include <sys/mount.h>

// TODO: Cleanup file/path handling

namespace lxcpp {

namespace fs = boost::filesystem;

void Provisions::execute()
{
    for(const auto & file : mConfig.mProvisions.files) {
        ProvisionFile(file).execute();
    }

    for(const auto & mount : mConfig.mProvisions.mounts) {
        ProvisionMount(mount).execute();
    }

    for(const auto & link : mConfig.mProvisions.links) {
        ProvisionLink(link).execute();
    }
}
void Provisions::revert()
{
    for(const auto & file : mConfig.mProvisions.files) {
        ProvisionFile(file).revert();
    }

    for(const auto & mount : mConfig.mProvisions.mounts) {
        ProvisionMount(mount).revert();
    }

    for(const auto & link : mConfig.mProvisions.links) {
        ProvisionLink(link).revert();
    }
}

ProvisionFile::ProvisionFile(const provision::File &file)
    : mFile(file)
{
    utils::assertIsAbsolute(file.path);
}

void ProvisionFile::execute()
{
    using namespace provision;

    switch (mFile.type) {
    case File::Type::DIRECTORY:
        utils::createDirs(mFile.path, mFile.mode);
        break;

    case File::Type::FIFO:
        utils::createFifo(mFile.path, mFile.mode);
        break;

    case File::Type::REGULAR:
        if ((mFile.flags & O_CREAT)) {
            utils::createFile(mFile.path, mFile.flags, mFile.mode);
        } else {
            utils::copyFile(mFile.path, mFile.path);
        }
        break;
    }
}
void ProvisionFile::revert()
{
    // TODO decision: should remove the file?
}


ProvisionMount::ProvisionMount(const provision::Mount &mount)
    : mMount(mount)
{
    utils::assertIsAbsolute(mount.target);
}

void ProvisionMount::execute()
{
    utils::mount(mMount.source, mMount.target, mMount.type, mMount.flags, mMount.data);
}
void ProvisionMount::revert()
{
    utils::umount(mMount.target, MNT_DETACH);
}


ProvisionLink::ProvisionLink(const provision::Link &link)
    : mLink(link)
{
    utils::assertIsAbsolute(link.target);
}

void ProvisionLink::execute()
{
    const std::string srcHostPath = fs::path(mLink.source).normalize().string();

    utils::createLink(srcHostPath, mLink.target);
}
void ProvisionLink::revert()
{
    // TODO decision: should remove the link?
}

} // namespace lxcpp
