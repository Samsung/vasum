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
#include "config/manager.hpp"
#include "vasum-client.h"

#include <boost/filesystem.hpp>

#include <string>
#include <fcntl.h>

namespace fs = boost::filesystem;

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
    config::loadFromKVStoreWithJsonFile(dbPath, configPath, mProvisioningConfig, dbPrefix);
}

ZoneProvision::~ZoneProvision()
{
    stop();
}

void ZoneProvision::saveProvisioningConfig()
{
    config::saveToKVStore(mDbPath, mProvisioningConfig, mDbPrefix);
}

void ZoneProvision::declareProvision(ZoneProvisioningConfig::Provision&& provision)
{
    mProvisioningConfig.provisions.push_back(std::move(provision));
    saveProvisioningConfig();
}

void ZoneProvision::declareFile(const int32_t& type,
                                const std::string& path,
                                const int32_t& flags,
                                const int32_t& mode)
{
    ZoneProvisioningConfig::Provision provision;
    provision.set(ZoneProvisioningConfig::File({type, path, flags, mode}));

    declareProvision(std::move(provision));
}

void ZoneProvision::declareMount(const std::string& source,
                                 const std::string& target,
                                 const std::string& type,
                                 const int64_t& flags,
                                 const std::string& data)
{
    ZoneProvisioningConfig::Provision provision;
    provision.set(ZoneProvisioningConfig::Mount({source, target, type, flags, data}));

    declareProvision(std::move(provision));
}

void ZoneProvision::declareLink(const std::string& source,
                                const std::string& target)
{
    ZoneProvisioningConfig::Provision provision;
    provision.set(ZoneProvisioningConfig::Link({source, target}));

    declareProvision(std::move(provision));
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

void ZoneProvision::file(const ZoneProvisioningConfig::File& config)
{
    bool ret = false;
    const fs::path hostPath = fs::path(mRootPath) / fs::path(config.path);
    switch (config.type) {
        case VSMFILE_DIRECTORY:
            ret = utils::createDirs(hostPath.string(), config.mode);
            if (!ret) {
                throw UtilsException("Can't create dir: " + hostPath.string());
            }
            break;

        case VSMFILE_FIFO:
            ret = utils::createFifo(hostPath.string(), config.mode);
            if (!ret) {
                throw UtilsException("Failed to make fifo: " + config.path);
            }
            break;

        case VSMFILE_REGULAR:
            if ((config.flags & O_CREAT)) {
                ret = utils::createFile(hostPath.string(), config.flags, config.mode);
                if (!ret) {
                    throw UtilsException("Failed to create file: " + config.path);
                }
            } else {
                ret = utils::copyFile(config.path, hostPath.string());
                if (!ret) {
                    throw UtilsException("Failed to copy file: " + config.path);
                }
            }
            break;
    }
}

void ZoneProvision::mount(const ZoneProvisioningConfig::Mount& config)
{
    const fs::path hostPath = fs::path(mRootPath) / fs::path(config.target);
    bool ret = utils::mount(config.source,
                            hostPath.string(),
                            config.type,
                            config.flags,
                            config.data);
    if (!ret) {
        throw UtilsException("Mount operation failure - source : " + config.source);
    }
}

void ZoneProvision::umount(const ZoneProvisioningConfig::Mount& config)
{
    const fs::path hostPath = fs::path(mRootPath) / fs::path(config.target);
    bool ret = utils::umount(hostPath.string());
    if (!ret) {
        throw UtilsException("Umount operation failure - path : " + config.target);
    }
}

void ZoneProvision::link(const ZoneProvisioningConfig::Link& config)
{
    const std::string srcHostPath = fs::path(config.source).normalize().string();
    for (const std::string& prefix : mValidLinkPrefixes) {
        if (prefix.length() <= srcHostPath.length()
            && srcHostPath.compare(0, prefix.length(), prefix) == 0) {

            const fs::path destHostPath = fs::path(mRootPath) / fs::path(config.target);
            bool ret = utils::createLink(srcHostPath, destHostPath.string());
            if (!ret) {
                throw UtilsException("Failed to create hard link: " +  config.source);
            }
            return;
        }
    }
    LOGE("Failed to create hard link: path=host: "
         << srcHostPath
         << ", msg: Path prefix is not valid path");
    throw UtilsException("Failed to hard link: path prefix is not valid");
}

} // namespace vasum
