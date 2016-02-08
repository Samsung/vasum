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

#include "config.hpp"

#include "lxcpp/commands/prep-guest-terminal.hpp"
#include "lxcpp/terminal.hpp"
#include "lxcpp/filesystem.hpp"

#include "utils/fd-utils.hpp"
#include "logger/logger.hpp"


namespace lxcpp {


PrepGuestTerminal::PrepGuestTerminal(PTYsConfig &terminals)
    : mTerminals(terminals)
{
}

PrepGuestTerminal::~PrepGuestTerminal()
{
}

void PrepGuestTerminal::execute()
{
    LOGD("Preparing " << mTerminals.mCount << " pseudoterminal(s) on the guest side.");

    // Bind mount some terminal devices from /dev/pts to /dev that are expected by applications.
    lxcpp::bindMountFile("/dev/pts/ptmx", "/dev/ptmx");
    lxcpp::bindMountFile("/dev/pts/0", "/dev/console");

    for (unsigned t = 0; t < mTerminals.mCount; ++t) {
        const std::string ptsPath = "/dev/pts/" + std::to_string(t);
        const std::string ttyPath = "/dev/tty" + std::to_string(t + 1);

        lxcpp::bindMountFile(ptsPath, ttyPath);
    }

    // Setup first PTY as a controlling terminal (/dev/console).
    // This way simple programs in the container can work
    // and we will be able to see the output of a container's init
    // before the launch of getty processes.
    int fd = utils::open("/dev/console",
                         O_RDWR | O_CLOEXEC | O_NOCTTY);
    setupIOControlTTY(fd);
}


} // namespace lxcpp
