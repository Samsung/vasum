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
 * @author  Dariusz Michaluk (d.michaluk@samsung.com)
 * @brief   Linux kernel parameters handling routines
 */

#include "config.hpp"

#include "exception.hpp"
#include "logger/logger.hpp"
#include "utils/fs.hpp"

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#ifdef USE_BOOST_REGEX
#include <boost/regex.hpp>
namespace rgx = boost;
#else
#include <regex>
namespace rgx = std;
#endif

namespace lxcpp {

void writeKernelParameter(const std::string& name, const std::string& value)
{
    if (name.empty() || value.empty()) {
        const std::string msg = "Kernel parameter name or value cannot be empty";
        LOGE(msg);
        throw BadArgument(msg);
    }

    // writes the value to a path under /proc/sys as determined from the name
    // for e.g. net.ipv4.ip_forward translated to /proc/sys/net/ipv4/ip_forward

    std::string namePath = "/proc/sys/" + rgx::regex_replace(name, rgx::regex("\\."), "/");

    if (!fs::exists(namePath)) {
        const std::string msg = "Kernel parameter: " + namePath + " does not exist";
        LOGE(msg);
        throw BadArgument(msg);
    }

    utils::saveFileContent(namePath, value);
}

std::string readKernelParameterValue(const std::string& name)
{
    if (name.empty()) {
        const std::string msg = "Kernel parameter name cannot be empty";
        LOGE(msg);
        throw BadArgument(msg);
    }

    std::string namePath = "/proc/sys/" + rgx::regex_replace(name, rgx::regex("\\."), "/");

    if (!fs::exists(namePath)) {
        const std::string msg = "Kernel parameter: " + namePath + " does not exist";
        LOGE(msg);
        throw BadArgument(msg);
    }

    std::string value;
    utils::readFirstLineOfFile(namePath, value);
    return value;
}

} // namespace lxcpp
