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
 * @brief   Declaration of the class for managing zone provision
 */


#ifndef SERVER_ZONE_PROVISION_HPP
#define SERVER_ZONE_PROVISION_HPP

#include "zone-provision-config.hpp"

#include <string>
#include <vector>
#include <list>

namespace vasum {


/**
 * Class is responsible for prepare filesystem for zone
 * It allows to create directories, files, mount points and copying files from host
 */
class ZoneProvision {

public:
    /**
     * ZoneProvision constructor
     * @param rootPath zone root path
     * @param configPath path to config with defaults
     * @param dbPath path to database
     * @param dbPrefix database prefix
     * @param validLinkPrefixes valid link prefixes
     */
    ZoneProvision(const std::string& rootPath,
                  const std::string& configPath,
                  const std::string& dbPath,
                  const std::string& dbPrefix,
                  const std::vector<std::string>& validLinkPrefixes);
    ~ZoneProvision();

    ZoneProvision(const ZoneProvision&) = delete;
    ZoneProvision& operator=(const ZoneProvision&) = delete;
    ZoneProvision(ZoneProvision&&) = default;

    /**
     * Declare file, directory or pipe that will be created while zone startup
     */
    std::string declareFile(const int32_t& type,
                            const std::string& path,
                            const int32_t& flags,
                            const int32_t& mode);
    /**
     * Declare mount that will be created while zone startup
     */
    std::string declareMount(const std::string& source,
                             const std::string& target,
                             const std::string& type,
                             const int64_t& flags,
                             const std::string& data);
    /**
     * Declare link that will be created while zone startup
     */
    std::string declareLink(const std::string& source,
                            const std::string& target);

    void start() noexcept;
    void stop() noexcept;

    /**
     * List all provisioned resources
     */
    std::vector<std::string> list() const;

    /**
     * Remove resource
     *
     * @param item resource to be removed (as in list())
     */
    void remove(const std::string& item);

private:
    ZoneProvisioningConfig mProvisioningConfig;
    std::string mRootPath;
    std::string mDbPath;
    std::string mDbPrefix;
    std::vector<std::string> mValidLinkPrefixes;
    std::list<ZoneProvisioningConfig::Provision> mProvisioned;

    void saveProvisioningConfig();
    std::string declareProvision(ZoneProvisioningConfig::Provision&& provision);

    void mount(const ZoneProvisioningConfig::Mount& config);
    void umount(const ZoneProvisioningConfig::Mount& config);
    void file(const ZoneProvisioningConfig::File& config);
    void link(const ZoneProvisioningConfig::Link& config);

    static std::string getId(const ZoneProvisioningConfig::File& file);
    static std::string getId(const ZoneProvisioningConfig::Mount& mount);
    static std::string getId(const ZoneProvisioningConfig::Link& link);
    static std::string getId(const ZoneProvisioningConfig::Provision& provision);
};

} // namespace vasum

#endif // SERVER_ZONE_PROVISION_HPP
