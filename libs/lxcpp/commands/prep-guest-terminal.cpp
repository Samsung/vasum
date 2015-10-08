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
 * @brief   Implementation of guest terminal preparation
 */

#include "lxcpp/commands/prep-guest-terminal.hpp"
#include "lxcpp/terminal.hpp"

#include "utils/fd-utils.hpp"
#include "logger/logger.hpp"


namespace lxcpp {


PrepGuestTerminal::PrepGuestTerminal(TerminalsConfig &terminals)
    : mTerminals(terminals)
{
}

PrepGuestTerminal::~PrepGuestTerminal()
{
}

void PrepGuestTerminal::execute()
{
    LOGD("Preparing " << mTerminals.count << " pseudoterminal(s) on the guest side.");

    // TODO when /dev/ is already namespaced, create:
    // /dev/pts/x (N times) (or mount devpts?)
    // symlink /dev/console -> first PTY
    // symlink /dev/ttyY -> /dev/pts/X (N times)
    //for (int i = 0; i < mTerminals.count; ++i) {}

    // Setup first PTY as a controlling terminal (/dev/console).
    // This way simple programs in the container can work
    // and we will be able to see the output of a container's init
    // before the launch of getty processes.
    int fd = utils::open(mTerminals.PTYs[0].ptsName,
                         O_RDWR | O_CLOEXEC | O_NOCTTY);
    setupIOControlTTY(fd);
}


} // namespace lxcpp
