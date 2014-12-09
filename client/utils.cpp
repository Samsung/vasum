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
const std::string CPUSET_LIBVIRT_PREFIX_OLD = "/machine/";
const std::string CPUSET_LIBVIRT_SUFFIX_OLD = ".libvirt-lxc";
const std::string CPUSET_LIBVIRT_PREFIX = "/machine.slice/machine-lxc\\x2d";
const std::string CPUSET_LIBVIRT_SUFFIX = ".scope";

bool parseLxcFormat(const std::string& cpuset, std::string& id)
{
    // /lxc/<id>
    if (!boost::starts_with(cpuset, CPUSET_LXC_PREFIX)) {
        return false;
    }
    id.assign(cpuset, CPUSET_LXC_PREFIX.size(), cpuset.size() - CPUSET_LXC_PREFIX.size());
    return true;
}

bool parseOldLibvirtFormat(const std::string& cpuset, std::string& id)
{
    // '/machine/<id>.libvirt-lxc'
    if (!boost::starts_with(cpuset, CPUSET_LIBVIRT_PREFIX_OLD)) {
        return false;
    }

    if (!boost::ends_with(cpuset, CPUSET_LIBVIRT_SUFFIX_OLD)) {
        return false;
    }

    id.assign(cpuset,
              CPUSET_LIBVIRT_PREFIX_OLD.size(),
              cpuset.size() - CPUSET_LIBVIRT_PREFIX_OLD.size() - CPUSET_LIBVIRT_SUFFIX_OLD.size());
    return true;
}

inline int unhex(char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return -1;
}

void unescape(std::string& value)
{
    const size_t len = value.size();
    size_t inPos = 0;
    size_t outPos = 0;
    while (inPos < len) {
        const char c = value[inPos++];
        if (c == '-') {
            value[outPos++] = '/';
        } else if (c == '\\' && value[inPos] == 'x') {
            const int a = unhex(value[inPos+1]);
            const int b = unhex(value[inPos+2]);
            value[outPos++] = (char) ((a << 4) | b);
            inPos += 3;
        } else {
            value[outPos++] = c;
        }
    }
    value.resize(outPos);
}

bool parseNewLibvirtFormat(const std::string& cpuset, std::string& id)
{
    // '/machine.slice/machine-lxc\x2d<id>.scope'
    if (!boost::starts_with(cpuset, CPUSET_LIBVIRT_PREFIX)) {
        return false;
    }

    if (!boost::ends_with(cpuset, CPUSET_LIBVIRT_SUFFIX)) {
        return false;
    }

    id = cpuset.substr(CPUSET_LIBVIRT_PREFIX.size(),
                       cpuset.size() - CPUSET_LIBVIRT_PREFIX.size() - CPUSET_LIBVIRT_SUFFIX.size());
    unescape(id);
    return true;
}

} // namespace

bool parseZoneIdFromCpuSet(const std::string& cpuset, std::string& id)
{
    if (cpuset == CPUSET_HOST) {
        id = "host";
        return true;
    }

    return parseLxcFormat(cpuset, id) ||
           parseNewLibvirtFormat(cpuset, id) ||
           parseOldLibvirtFormat(cpuset, id);
}

