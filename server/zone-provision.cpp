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

namespace {

const std::string ZONE_PROVISION_FILE = "provision.conf";

void declareUnit(const std::string& file, ZoneProvisioning::Unit&& unit)
{
     // TODO: Add to the dynamic configuration
    ZoneProvisioning config;
    if (fs::exists(file)) {
        config::loadFromJsonFile(file, config);
    }
    config.units.push_back(std::move(unit));
    config::saveToJsonFile(file, config);
}

} // namespace

ZoneProvision::ZoneProvision(const std::string& zonePath,
                             const std::vector<std::string>& validLinkPrefixes)
{
    mProvisionFile = (fs::path(zonePath) / fs::path(ZONE_PROVISION_FILE)).string();
    mRootPath = (zonePath / fs::path("rootfs")).string();
    mValidLinkPrefixes = validLinkPrefixes;
}

ZoneProvision::~ZoneProvision()
{
    stop();
}

std::string ZoneProvision::getRootPath() const
{
    return mRootPath;
}


void ZoneProvision::declareFile(const int32_t& type,
                                const std::string& path,
                                const int32_t& flags,
                                const int32_t& mode)
{
    ZoneProvisioning::Unit unit;
    unit.set(ZoneProvisioning::File({type, path, flags, mode}));

    declareUnit(mProvisionFile, std::move(unit));
}

void ZoneProvision::declareMount(const std::string& source,
                                 const std::string& target,
                                 const std::string& type,
                                 const int64_t& flags,
                                 const std::string& data)
{
    ZoneProvisioning::Unit unit;
    unit.set(ZoneProvisioning::Mount({source, target, type, flags, data}));

    declareUnit(mProvisionFile, std::move(unit));
}

void ZoneProvision::declareLink(const std::string& source,
                                const std::string& target)
{
    ZoneProvisioning::Unit unit;
    unit.set(ZoneProvisioning::Link({source, target}));

    declareUnit(mProvisionFile, std::move(unit));
}

void ZoneProvision::start() noexcept
{
    if (fs::exists(mProvisionFile)) {
        config::loadFromJsonFile(mProvisionFile, mProvisioningConfig);
        for (const auto& unit : mProvisioningConfig.units) {
            try {
                if (unit.is<ZoneProvisioning::File>()) {
                    file(unit.as<ZoneProvisioning::File>());
                } else if (unit.is<ZoneProvisioning::Mount>()) {
                    mount(unit.as<ZoneProvisioning::Mount>());
                } else if (unit.is<ZoneProvisioning::Link>()) {
                    link(unit.as<ZoneProvisioning::Link>());
                }
                // mProvisioned must be FILO
                mProvisioned.push_front(unit);
            } catch (const std::exception& ex) {
                LOGE("Provsion error: " << ex.what());
            }
        }
    }
}

void ZoneProvision::stop() noexcept
{
    mProvisioned.remove_if([this](const ZoneProvisioning::Unit& unit) -> bool {
        try {
            if (unit.is<ZoneProvisioning::Mount>()) {
                umount(unit.as<ZoneProvisioning::Mount>());
            }
            // leaves files, links, fifo, untouched
            return true;
        } catch (const std::exception& ex) {
            LOGE("Provsion error: " << ex.what());
            return false;
        }
    });
}

void ZoneProvision::file(const ZoneProvisioning::File& config)
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

void ZoneProvision::mount(const ZoneProvisioning::Mount& config)
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

void ZoneProvision::umount(const ZoneProvisioning::Mount& config)
{
    const fs::path hostPath = fs::path(mRootPath) / fs::path(config.target);
    bool ret = utils::umount(hostPath.string());
    if (!ret) {
        throw UtilsException("Umount operation failure - path : " + config.target);
    }
}

void ZoneProvision::link(const ZoneProvisioning::Link& config)
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
