
/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Mateusz Malicki <m.malicki2@samsung.com>
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
 * @author  Mateusz Malicki (m.malicki2@samsung.com)
 * @brief   Implementation of class for managing zone provsion
 */

#include "config.hpp"

#include "zone-provision.hpp"
#include "zone-provision-config.hpp"

#include "logger/logger.hpp"
#include "utils/fs.hpp"
#include "utils/exception.hpp"
#include "lxcpp/exception.hpp"
#include "cargo-sqlite/cargo-sqlite.hpp"
#include "cargo-sqlite-json/cargo-sqlite-json.hpp"
#include "vasum-client.h"

#include <boost/filesystem.hpp>

#include <string>
#include <algorithm>
#include <fcntl.h>

namespace fs = boost::filesystem;

using namespace utils;

namespace vasum {

ZoneProvision::ZoneProvision(const std::string& rootPath,
                             const std::string& configPath,
                             const std::string& dbPath,
                             const std::string& dbPrefix,
                             const std::vector<std::string>& validLinkPrefixes)
    : mRootPath(rootPath)
    , mDbPath(dbPath)
    , mDbPrefix(dbPrefix)
    , mValidLinkPrefixes(validLinkPrefixes)
{
    cargo::loadFromKVStoreWithJsonFile(dbPath, configPath, mProvisioningConfig, dbPrefix);
}

ZoneProvision::~ZoneProvision()
{
    stop();
}

void ZoneProvision::saveProvisioningConfig()
{
    cargo::saveToKVStore(mDbPath, mProvisioningConfig, mDbPrefix);
}

std::string ZoneProvision::declareProvision(ZoneProvisioningConfig::Provision&& provision)
{
    std::string id = getId(provision);
    auto it = std::find_if(mProvisioningConfig.provisions.begin(),
                           mProvisioningConfig.provisions.end(),
                           [&](const ZoneProvisioningConfig::Provision& existingProvision) {
                               return getId(existingProvision) == id;
                           });
    if (it != mProvisioningConfig.provisions.end()) {
        const std::string msg = "Can't add provision. It already exists: " + id;
        LOGE(msg);
        throw lxcpp::ProvisionExistsException(msg);
    }
    mProvisioningConfig.provisions.push_back(std::move(provision));
    saveProvisioningConfig();
    return id;
}

std::string ZoneProvision::declareFile(const int32_t& type,
                                       const std::string& path,
                                       const int32_t& flags,
                                       const int32_t& mode)
{
    ZoneProvisioningConfig::Provision provision;
    provision.set(ZoneProvisioningConfig::File({type, path, flags, mode}));

    return declareProvision(std::move(provision));
}

std::string ZoneProvision::declareMount(const std::string& source,
                                        const std::string& target,
                                        const std::string& type,
                                        const int64_t& flags,
                                        const std::string& data)
{
    ZoneProvisioningConfig::Provision provision;
    provision.set(ZoneProvisioningConfig::Mount({source, target, type, flags, data}));

    return declareProvision(std::move(provision));
}

std::string ZoneProvision::declareLink(const std::string& source,
                                       const std::string& target)
{
    ZoneProvisioningConfig::Provision provision;
    provision.set(ZoneProvisioningConfig::Link({source, target}));

    return declareProvision(std::move(provision));
}

void ZoneProvision::start() noexcept
{
    for (const auto& provision : mProvisioningConfig.provisions) {
        try {
            if (provision.is<ZoneProvisioningConfig::File>()) {
                file(provision.as<ZoneProvisioningConfig::File>());
            } else if (provision.is<ZoneProvisioningConfig::Mount>()) {
                mount(provision.as<ZoneProvisioningConfig::Mount>());
            } else if (provision.is<ZoneProvisioningConfig::Link>()) {
                link(provision.as<ZoneProvisioningConfig::Link>());
            }
            // mProvisioned must be FILO
            mProvisioned.push_front(provision);
        } catch (const std::exception& ex) {
            LOGE("Provsion error: " << ex.what());
        }
    }
}

void ZoneProvision::stop() noexcept
{
    mProvisioned.remove_if([this](const ZoneProvisioningConfig::Provision& provision) -> bool {
        try {
            if (provision.is<ZoneProvisioningConfig::Mount>()) {
                umount(provision.as<ZoneProvisioningConfig::Mount>());
            }
            // leaves files, links, fifo, untouched
            return true;
        } catch (const std::exception& ex) {
            LOGE("Provsion error: " << ex.what());
            return false;
        }
    });
}

std::vector<std::string> ZoneProvision::list() const
{
    std::vector<std::string> items;
    for (const auto& provision : mProvisioningConfig.provisions) {
        items.push_back(getId(provision));
    }
    return items;
}

void ZoneProvision::remove(const std::string& item)
{

    const auto it = std::find_if(mProvisioningConfig.provisions.begin(),
                                 mProvisioningConfig.provisions.end(),
                                 [&](const ZoneProvisioningConfig::Provision& provision) {
                                    return getId(provision) == item;
                                 });
    if (it == mProvisioningConfig.provisions.end()) {
        const std::string msg = "Can't remove provision: not found.";
        LOGE(msg);
        throw lxcpp::ProvisionNotFoundException(msg);
    }

    mProvisioningConfig.provisions.erase(it);
    LOGI("Provision removed: " << item);
}

void ZoneProvision::file(const ZoneProvisioningConfig::File& config)
{
    const fs::path hostPath = fs::path(mRootPath) / fs::path(config.path);
    switch (config.type) {
        case VSMFILE_DIRECTORY:
            utils::createDirs(hostPath.string(), config.mode);
            break;

        case VSMFILE_FIFO:
            utils::createFifo(hostPath.string(), config.mode);
            break;

        case VSMFILE_REGULAR:
            if ((config.flags & O_CREAT)) {
                utils::createFile(hostPath.string(), config.flags, config.mode);
            } else {
                utils::copyFile(config.path, hostPath.string());
            }
            break;
    }
}

void ZoneProvision::mount(const ZoneProvisioningConfig::Mount& config)
{
    const fs::path hostPath = fs::path(mRootPath) / fs::path(config.target);
    utils::mount(config.source, hostPath.string(), config.type, config.flags, config.data);
}

void ZoneProvision::umount(const ZoneProvisioningConfig::Mount& config)
{
    const fs::path hostPath = fs::path(mRootPath) / fs::path(config.target);
    utils::umount(hostPath.string());
}

void ZoneProvision::link(const ZoneProvisioningConfig::Link& config)
{
    const std::string srcHostPath = fs::path(config.source).normalize().string();
    for (const std::string& prefix : mValidLinkPrefixes) {
        if (prefix.length() <= srcHostPath.length()
            && srcHostPath.compare(0, prefix.length(), prefix) == 0) {

            const fs::path destHostPath = fs::path(mRootPath) / fs::path(config.target);
            utils::createLink(srcHostPath, destHostPath.string());
            return;
        }
    }
    THROW_EXCEPTION(UtilsException, "Failed to create hard link: path=host: " +
                    srcHostPath + ", msg: Path prefix is not valid path");
}

std::string ZoneProvision::getId(const ZoneProvisioningConfig::File& file)
{
    //TODO output of type,flags and mode should be more user friendly
    return "file " +
           file.path + " " +
           std::to_string(file.type) + " " +
           std::to_string(file.flags) + " " +
           std::to_string(file.mode);
}

std::string ZoneProvision::getId(const ZoneProvisioningConfig::Mount& mount)
{
    //TODO output of flags should be more user friendly
    return "mount " +
           mount.source + " " +
           mount.target + " " +
           mount.type + " " +
           std::to_string(mount.flags) + " " +
           mount.data;
}

std::string ZoneProvision::getId(const ZoneProvisioningConfig::Link& link)
{
    return "link " + link.source + " " + link.target;
}

std::string ZoneProvision::getId(const ZoneProvisioningConfig::Provision& provision)
{
    using namespace vasum;
    if (provision.is<ZoneProvisioningConfig::File>()) {
        return getId(provision.as<ZoneProvisioningConfig::File>());
    } else if (provision.is<ZoneProvisioningConfig::Mount>()) {
        return getId(provision.as<ZoneProvisioningConfig::Mount>());
    } else if (provision.is<ZoneProvisioningConfig::Link>()) {
        return getId(provision.as<ZoneProvisioningConfig::Link>());
    }
    const std::string msg = "Provision type not supported";
    LOGE(msg);
    throw lxcpp::ProvisionNotSupportedException(msg);

}

} // namespace vasum
