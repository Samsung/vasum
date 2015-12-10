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
 * @brief   Implementation of the console
 */

#include "config.hpp"

#include "lxcpp/commands/console.hpp"
#include "lxcpp/exception.hpp"
#include "lxcpp/terminal.hpp"
#include "lxcpp/credentials.hpp"

#include "cargo-ipc/epoll/event-poll.hpp"
#include "logger/logger.hpp"
#include "utils/fd-utils.hpp"
#include "utils/signal.hpp"

#include <unistd.h>
#include <functional>
#include <iostream>
#include <cstring>
#include <sys/ioctl.h>


namespace lxcpp {


Console::Console(TerminalsConfig &terminals, unsigned int terminalNum)
    : mTerminals(terminals),
      mTerminalNum(terminalNum),
      mServiceMode(false),
      mQuitReason(ConsoleQuitReason::NONE),
      mEventPoll(),
      mSignalFD(mEventPoll),
      appToTermOffset(0),
      termToAppOffset(0)
{
    if (terminalNum >= terminals.count) {
        const std::string msg = "Requested terminal number does not exist";
        LOGE(msg);
        throw TerminalException(msg);
    }
}

Console::~Console()
{
}

void Console::execute()
{
    if (!lxcpp::isatty(STDIN_FILENO) || !lxcpp::isatty(STDOUT_FILENO)) {
        const std::string msg = "Standard input is not a terminal, cannot launch the console";
        LOGE(msg);
        throw TerminalException(msg);
    }

    LOGD("Launching the console with: " << mTerminals.count << " pseudoterminal(s) on the guest side.");
    std::cout << "Connected to the zone, escape characted is ^] or ^a q." << std::endl;
    std::cout << "If the container has just a shell remember to set TERM to be equal to the one of your own terminal." << std::endl;
    std::cout << "Terminal number: " << mTerminalNum << ", use ^a n/p to switch between them." << std::endl;

    setupTTY();
    resizePTY();

    using namespace std::placeholders;
    mEventPoll.addFD(STDIN_FILENO, EPOLLIN, std::bind(&Console::onStdInput, this, _1, _2));
    mEventPoll.addFD(STDOUT_FILENO, 0, std::bind(&Console::onStdOutput, this, _1, _2));
    mEventPoll.addFD(getCurrentFD(), EPOLLIN, std::bind(&Console::onPTY, this, _1, _2));

    while (mQuitReason == ConsoleQuitReason::NONE) {
        mEventPoll.dispatchIteration(-1);
    }

    mEventPoll.removeFD(getCurrentFD());
    mEventPoll.removeFD(STDIN_FILENO);
    mEventPoll.removeFD(STDOUT_FILENO);

    restoreTTY();

    switch (mQuitReason) {
    case ConsoleQuitReason::USER:
        std::cout << std::endl << "User requested quit" << std::endl;
        break;
    case ConsoleQuitReason::ERR:
        std::cout << std::endl << "There has been an error on the terminal, quitting" << std::endl;
        break;
    case ConsoleQuitReason::HUP:
        std::cout << std::endl << "Terminal disconnected, quitting" << std::endl;
        break;
    default:
        std::cout << std::endl << "Unknown error, quitting" << std::endl;
        break;
    }

    // make the class reusable with subsequent execute()s
    mQuitReason = ConsoleQuitReason::NONE;
    mServiceMode = false;
}

void Console::setupTTY()
{
    // save signal state, ignore several signal, setup resize window signal
    mSignalStates = utils::signalIgnore({SIGQUIT, SIGTERM, SIGINT, SIGHUP, SIGPIPE, SIGWINCH});
    mSignalFD.setHandler(SIGWINCH, std::bind(&Console::resizePTY, this));

    // save terminal state
    lxcpp::tcgetattr(STDIN_FILENO, &mTTYState);

    // set the current terminal in raw mode:
    struct termios newTTY = mTTYState;
    ::cfmakeraw(&newTTY);
    lxcpp::tcsetattr(STDIN_FILENO, TCSAFLUSH, &newTTY);
}

void Console::resizePTY()
{
    // resize the underlying PTY terminal to the size of the user terminal
    struct winsize wsz;
    utils::ioctl(STDIN_FILENO, TIOCGWINSZ, &wsz);
    utils::ioctl(getCurrentFD(), TIOCSWINSZ, &wsz);
}

void Console::restoreTTY()
{
    // restore signal state
    for (const auto sigInfo : mSignalStates) {
        utils::signalSet(sigInfo.first, &sigInfo.second);
    }

    // restore terminal state
    lxcpp::tcsetattr(STDIN_FILENO, TCSAFLUSH, &mTTYState);
}

void Console::onPTY(int fd, cargo::ipc::epoll::Events events)
{
    if ((events & EPOLLIN) == EPOLLIN) {
        const size_t avail = IO_BUFFER_SIZE - appToTermOffset;
        char *buf = appToTerm + appToTermOffset;

        const ssize_t read = ::read(fd, buf, avail);
        appToTermOffset += read;

        if (appToTermOffset) {
            mEventPoll.modifyFD(STDOUT_FILENO, EPOLLOUT);
        }
    }

    if ((events & EPOLLOUT) == EPOLLOUT && termToAppOffset) {
        const ssize_t written = ::write(fd, termToApp, termToAppOffset);
        ::memmove(termToApp, termToApp + written, termToAppOffset - written);
        termToAppOffset -= written;

        if (termToAppOffset == 0) {
            mEventPoll.modifyFD(fd, EPOLLIN);
        }
    }

    checkForError(events);
}

void Console::onStdInput(int fd, cargo::ipc::epoll::Events events)
{
    if ((events & EPOLLIN) == EPOLLIN) {
        const size_t avail = IO_BUFFER_SIZE - termToAppOffset;
        char *buf = termToApp + termToAppOffset;
        const ssize_t read = ::read(fd, buf, avail);

        if (read == 1 && handleSpecial(buf[0])) {
            return;
        }

        termToAppOffset += read;

        if (termToAppOffset) {
            mEventPoll.modifyFD(getCurrentFD(), EPOLLIN | EPOLLOUT);
        }
    }

    checkForError(events);
}

void Console::onStdOutput(int fd, cargo::ipc::epoll::Events events)
{
    if ((events & EPOLLOUT) == EPOLLOUT && appToTermOffset) {
        const ssize_t written = ::write(fd, appToTerm, appToTermOffset);
        ::memmove(appToTerm, appToTerm + written, appToTermOffset - written);
        appToTermOffset -= written;

        if (appToTermOffset == 0) {
            mEventPoll.modifyFD(fd, 0);
        }
    }

    checkForError(events);
}

void Console::checkForError(cargo::ipc::epoll::Events events)
{
    // TODO: ignore EPOLLHUP for now, this allows us to cycle through not
    // connected terminals. When we can handle full containers with getty()
    // processes we'll decide what to do about that.
#if 0
    if ((events & EPOLLHUP) == EPOLLHUP) {
        mQuitReason = ConsoleQuitReason::HUP;
    }
#endif
    if ((events & EPOLLERR) == EPOLLERR) {
        mQuitReason = ConsoleQuitReason::ERR;
    }
}

bool Console::handleSpecial(const char key)
{
    if (mServiceMode) {
        mServiceMode = false;

        switch(key) {
        case 'q':
            mQuitReason = ConsoleQuitReason::USER;
            return true;
        case 'n':
            consoleChange(ConsoleChange::NEXT);
            return true;
        case 'p':
            consoleChange(ConsoleChange::PREV);
            return true;
        default:
            return true;
        }
    }

    if (key == 0x1d) { // ^]
        mQuitReason = ConsoleQuitReason::USER;
        return true;
    }
    if (key == 0x01) { // ^a
        mServiceMode = true;
        return true;
    }

    return false;
}

void Console::consoleChange(ConsoleChange direction)
{
    mEventPoll.removeFD(getCurrentFD());

    if (direction == ConsoleChange::NEXT) {
        ++mTerminalNum;
    } else if (direction == ConsoleChange::PREV) {
        --mTerminalNum;
    }

    mTerminalNum = (mTerminalNum + mTerminals.count) % mTerminals.count;

    int mode = EPOLLIN;
    if (termToAppOffset) {
        mode &= EPOLLOUT;
    }

    restoreTTY();
    std::cout << "Terminal number: " << mTerminalNum << std::endl;
    setupTTY();
    resizePTY();

    using namespace std::placeholders;
    mEventPoll.addFD(getCurrentFD(), mode, std::bind(&Console::onPTY, this, _1, _2));
}

int Console::getCurrentFD() const
{
    return mTerminals.PTYs[mTerminalNum].masterFD.value;
}


} // namespace lxcpp
