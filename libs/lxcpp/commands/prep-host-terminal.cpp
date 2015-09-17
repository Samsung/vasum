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

#include "lxcpp/commands/prep-host-terminal.hpp"
#include "lxcpp/terminal.hpp"

#include "logger/logger.hpp"


namespace lxcpp {


PrepHostTerminal::PrepHostTerminal(TerminalsConfig &terminals)
    : mTerminals(terminals)
{
}

PrepHostTerminal::~PrepHostTerminal()
{
}

void PrepHostTerminal::execute()
{
    LOGD("Creating " << mTerminals.count << " pseudoterminal(s) on the host side:");

    for (int i = 0; i < mTerminals.count; ++i)
    {
        const auto pty = lxcpp::openPty(true);
        LOGD(pty.second << " has been created");
        mTerminals.PTYs.emplace_back(pty.first, pty.second);
    }
}


} // namespace lxcpp
