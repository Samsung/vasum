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
     * @param zonePath directory where zones are defined (lxc configs, rootfs etc)
     */
    ZoneProvision(const std::string& zonePath,
                  const std::vector<std::string>& validLinkPrefixes);
    ~ZoneProvision();

    /**
     * Declare file, directory or pipe that will be created while zone startup
     */
    void declareFile(const int32_t& type,
                     const std::string& path,
                     const int32_t& flags,
                     const int32_t& mode);
    /**
     * Declare mount that will be created while zone startup
     */
    void declareMount(const std::string& source,
                      const std::string& target,
                      const std::string& type,
                      const int64_t& flags,
                      const std::string& data);
    /**
     * Declare link that will be created while zone startup
     */
    void declareLink(const std::string& source,
                     const std::string& target);

   /**
     * Get zone root path
     */
    std::string getRootPath() const;

    void start() noexcept;
    void stop() noexcept;

private:
    ZoneProvisioning mProvisioningConfig;
    std::string mRootPath;
    std::string mProvisionFile;
    std::vector<std::string> mValidLinkPrefixes;
    std::list<ZoneProvisioning::Unit> mProvisioned;

    void mount(const ZoneProvisioning::Mount& config);
    void umount(const ZoneProvisioning::Mount& config);
    void file(const ZoneProvisioning::File& config);
    void link(const ZoneProvisioning::Link& config);
};


} // namespace vasum

#endif // SERVER_ZONE_PROVISION_HPP
