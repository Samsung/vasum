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

namespace lxcpp {

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


void ProvisionFile::execute()
{
    // MJK TODO: add file
}
void ProvisionFile::revert()
{
    // MJK TODO: remove file from container
}


void ProvisionMount::execute()
{
    // MJK TODO: add mount
}
void ProvisionMount::revert()
{
    // MJK TODO: remove mount from container
}


void ProvisionLink::execute()
{
    // MJK TODO: add link
}
void ProvisionLink::revert()
{
    // MJK TODO: remove link from container
}

} // namespace lxcpp
