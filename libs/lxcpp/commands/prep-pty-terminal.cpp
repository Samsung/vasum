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
 * @author  Lukasz Pawelczyk (l.pawelczyk@samsung.com)
 * @brief   Implementation of host terminal preparation
 */

#include "config.hpp"

#include "lxcpp/commands/prep-pty-terminal.hpp"
#include "lxcpp/filesystem.hpp"
#include "lxcpp/terminal.hpp"

#include "utils/fd-utils.hpp"
#include "utils/paths.hpp"
#include "logger/logger.hpp"


namespace lxcpp {


PrepPTYTerminal::PrepPTYTerminal(PTYsConfig &terminals)
    : mTerminals(terminals)
{
}

PrepPTYTerminal::~PrepPTYTerminal()
{
}

void PrepPTYTerminal::execute()
{
    LOGD("Creating " << mTerminals.mCount << " pseudoterminal(s):");

    for (unsigned int i = 0; i < mTerminals.mCount; ++i) {
        std::pair<int, std::string> pty;

        if (!mTerminals.mDevptsPath.empty()) {
            const std::string ptmxPath = utils::createFilePath(mTerminals.mDevptsPath,
                                                               "ptmx");
            pty = lxcpp::openPty(ptmxPath);

            const std::string slavePath = utils::createFilePath(mTerminals.mDevptsPath,
                                                                pty.second);
            lxcpp::chown(slavePath, mTerminals.mUID, -1);
        } else {
            pty = lxcpp::openPty(true);
        }

        LOGD("Terminal: " << pty.second << " has been created");
        mTerminals.mPTYs.emplace_back(pty.first, pty.second);
    }
}

void PrepPTYTerminal::revert()
{
    LOGD("Closing " << mTerminals.mCount << " pseudoterminal(s).");

    for (const auto &it : mTerminals.mPTYs) {
        LOGD("Terminal: " << it.mPtsName << " has been closed");
        utils::close(it.mMasterFD.value);
    }
}


} // namespace lxcpp
