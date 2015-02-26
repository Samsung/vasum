/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Piotr Bartosiewicz <p.bartosiewi@partner.samsung.com>
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
 * @author  Piotr Bartosiewicz (p.bartosiewi@partner.samsung.com)
 * @brief   Lxc cgroup stuff
 */

#ifndef COMMON_LXC_CGROUP_HPP
#define COMMON_LXC_CGROUP_HPP

#include <string>

namespace vasum {
namespace lxc {

bool setCgroup(const std::string& zoneName,
               const std::string& cgroupController,
               const std::string& cgroupName,
               const std::string& value);

bool getCgroup(const std::string& zoneName,
               const std::string& cgroupController,
               const std::string& cgroupName,
               std::string& value);

bool isDevice(const std::string& device);

bool setDeviceAccess(const std::string& zoneName,
                     const std::string& device,
                     bool grant,
                     uint32_t flags);


} // namespace lxc
} // namespace vasum


#endif // COMMON_LXC_CGROUP_HPP
