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
 * @brief   Headers for the console
 */

#ifndef LXCPP_COMMANDS_CONSOLE_HPP
#define LXCPP_COMMANDS_CONSOLE_HPP

#include "lxcpp/commands/command.hpp"
#include "lxcpp/terminal-config.hpp"
#include "lxcpp/terminal.hpp"

#include "cargo-ipc/epoll/event-poll.hpp"
#include "utils/signalfd.hpp"

#include <signal.h>
#include <array>


namespace lxcpp {


class Console final: Command {
public:
    /**
     * Launches the console on the current terminal
     *
     * @param terminals    container's terminals config
     * @param terminalNum  initial terminal to attach to
     */
    Console(TerminalsConfig &terminals, unsigned int terminalNum = 0);
    ~Console();

    void execute();

private:
    enum class ConsoleQuitReason : int {
        NONE = 0,
        USER = 1,
        HUP = 2,
        ERR = 3
    };
    enum class ConsoleChange : int {
        NEXT = 0,
        PREV = 1
    };
    static const int IO_BUFFER_SIZE = 1024;

    TerminalsConfig &mTerminals;
    int mTerminalNum;
    bool mServiceMode;
    ConsoleQuitReason mQuitReason;
    cargo::ipc::epoll::EventPoll mEventPoll;
    utils::SignalFD mSignalFD;
    std::vector<std::pair<int, struct ::sigaction>> mSignalStates;
    struct termios mTTYState;

    char appToTerm[IO_BUFFER_SIZE];
    int appToTermOffset;
    char termToApp[IO_BUFFER_SIZE];
    int termToAppOffset;

    void setupTTY();
    void restoreTTY();
    void resizePTY();
    void onPTY(int fd, cargo::ipc::epoll::Events events);
    void onStdInput(int fd, cargo::ipc::epoll::Events events);
    void onStdOutput(int fd, cargo::ipc::epoll::Events events);
    void checkForError(cargo::ipc::epoll::Events events);
    bool handleSpecial(char key);
    void consoleChange(ConsoleChange direction);
    int getCurrentFD() const;
};


} // namespace lxcpp


#endif // LXCPP_COMMANDS_CONSOLE_HPP
