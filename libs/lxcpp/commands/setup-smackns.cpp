/*
 *  Copyright (C) 2016 Samsung Electronics Co., Ltd All Rights Reserved
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
 * @author  Lukasz Kostyra (l.kostyra@samsung.com)
 * @brief   Definitions of SetupSmackNS Command
 */

#include "config.hpp"

#include "lxcpp/commands/setup-smackns.hpp"

#include "utils/fs.hpp"
#include "logger/logger.hpp"


namespace lxcpp {


SetupSmackNS::SetupSmackNS(const SmackNSConfig &smackNSConfig, const pid_t initPid)
    : mSmackNSConfig(smackNSConfig),
      mInitPid(initPid)
{
}

SetupSmackNS::~SetupSmackNS()
{
}

void SetupSmackNS::execute()
{
    if (mSmackNSConfig.mLabelMap.empty()) {
        return; // nothing to add, exit the setup
    }

    const std::string labelMapPath = "/proc/" + std::to_string(mInitPid) + "/attr/label_map";
    if (!utils::exists(labelMapPath)) {
        const std::string msg = "Unable to configure Smack namespace - " + labelMapPath
                              + " unreachable. Probably the kernel does not support Smack NS.";
        LOGE(msg);
        throw SmackNSException(msg);
    }

    std::string labelMap;
    for (const auto &mapping : mSmackNSConfig.mLabelMap) {
        labelMap.append(std::get<0>(mapping) + " " + std::get<1>(mapping) + "\n");
    }
    if (!labelMap.empty() && !utils::saveFileContent(labelMapPath, labelMap)) {
        const std::string msg = "Failed to write the Smack label map";
        LOGE(msg);
        throw SmackNSException(msg);
    }
}


} // namespace lxcpp
