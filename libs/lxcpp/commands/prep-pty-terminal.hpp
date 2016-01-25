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
 * @brief   Headers for host terminal preparation
 */

#ifndef LXCPP_COMMANDS_PREP_HOST_TERMINAL_HPP
#define LXCPP_COMMANDS_PREP_HOST_TERMINAL_HPP

#include "lxcpp/commands/command.hpp"
#include "lxcpp/pty-config.hpp"


namespace lxcpp {


class PrepPTYTerminal final: Command {
public:
    /**
     * Prepares the terminal on the host side.
     *
     * It creates a number of pseudoterminals and stores them
     * to be passed to the guard and prepared for init process.
     *
     * @param terminals  container's terminals config
     */
    PrepPTYTerminal(PTYsConfig &terminals);
    ~PrepPTYTerminal();

    void execute();
    void revert();

private:
    PTYsConfig &mTerminals;
};


} // namespace lxcpp


#endif // LXCPP_COMMANDS_PREP_HOST_TERMINAL_HPP
