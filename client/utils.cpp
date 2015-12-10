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
 * @brief   Utility functions definition
 */

#include "config.hpp"

#include "utils.hpp"

#include <boost/algorithm/string.hpp>

namespace {

const std::string CPUSET_HOST = "/";
const std::string CPUSET_LXC_PREFIX = "/lxc/";

bool parseLxcFormat(const std::string& cpuset, std::string& id)
{
    // /lxc/<id>
    if (!boost::starts_with(cpuset, CPUSET_LXC_PREFIX)) {
        return false;
    }
    id.assign(cpuset, CPUSET_LXC_PREFIX.size(), cpuset.size() - CPUSET_LXC_PREFIX.size());
    return true;
}

} // namespace

bool parseZoneIdFromCpuSet(const std::string& cpuset, std::string& id)
{
    if (cpuset == CPUSET_HOST) {
        id = "host";
        return true;
    }

    return parseLxcFormat(cpuset, id);
}

