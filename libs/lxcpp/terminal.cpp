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
 * @brief   Terminal helpers implementation
 */

#include "config.hpp"

#include "lxcpp/exception.hpp"

#include "logger/logger.hpp"
#include "utils/exception.hpp"
#include "utils/fd-utils.hpp"
#include "utils/credentials.hpp"

#include <string>
#include <pty.h>
#include <limits.h>
#include <unistd.h>


namespace lxcpp {


namespace {

void openpty(int *master, int *slave)
{
    // Do not use other parameters, they are not 100% safe
    if (-1 == ::openpty(master, slave, NULL, NULL, NULL)) {
        const std::string msg = "openpty() failed: " + utils::getSystemErrorMessage();
        LOGE(msg);
        throw TerminalException(msg);
    }
}

std::string ttyname_r(const int fd)
{
    char ptsName[PATH_MAX];
    int rc = ::ttyname_r(fd, ptsName, PATH_MAX);

    if (rc != 0) {
        const std::string msg = "ttyname_r() failed: " + utils::getSystemErrorMessage(rc);
        LOGE(msg);
        throw TerminalException(msg);
    }

    return ptsName;
}

} // namespace


/*
 * This function has to be safe in regard to signal(7)
 */
int nullStdFDs()
{
    int ret = -1;

    int fd = TEMP_FAILURE_RETRY(::open("/dev/null", O_RDWR | O_CLOEXEC));
    if (fd == -1) {
        goto err;
    }

    if (-1 == TEMP_FAILURE_RETRY(::dup2(fd, STDIN_FILENO))) {
        goto err_close;
    }

    if (-1 == TEMP_FAILURE_RETRY(::dup2(fd, STDOUT_FILENO))) {
        goto err_close;
    }

    if (-1 == TEMP_FAILURE_RETRY(::dup2(fd, STDERR_FILENO))) {
        goto err_close;
    }

    if (-1 == TEMP_FAILURE_RETRY(::close(fd))) {
        goto err;
    }

    return 0;

err_close:
    TEMP_FAILURE_RETRY(::close(fd));
err:
    return ret;
}

bool isatty(int fd)
{
    int ret = ::isatty(fd);
    if (ret) {
        return true;
    }
    if (errno == EINVAL || errno == ENOTTY) {
        return false;
    }

    const std::string msg = "isatty() failed: " + utils::getSystemErrorMessage();
    LOGE(msg);
    throw TerminalException(msg);
}

void tcgetattr(const int fd, struct ::termios *termios_p)
{
    if (-1 == ::tcgetattr(fd, termios_p)) {
        const std::string msg = "tcgetattr() failed: " + utils::getSystemErrorMessage();
        LOGE(msg);
        throw TerminalException(msg);
    }
}

void tcsetattr(const int fd, const int optional_actions, const struct ::termios *termios_p)
{
    if (-1 == ::tcsetattr(fd, optional_actions, termios_p)) {
        const std::string msg = "tcsetattr() failed: " + utils::getSystemErrorMessage();
        LOGE(msg);
        throw TerminalException(msg);
    }
}

struct ::termios makeRawTerm(int fd)
{
    struct termios ttyAttr, ret;

    lxcpp::tcgetattr(fd, &ttyAttr);
    ret = ttyAttr;

    ::cfmakeraw(&ttyAttr);
    lxcpp::tcsetattr(fd, TCSADRAIN, &ttyAttr);

    return ret;
}


void setupIOControlTTY(const int ttyFD)
{
    if (!lxcpp::isatty(ttyFD)) {
        const std::string msg = "setupIOControlTTY(): file descriptor passed is not a terminal";
        LOGE(msg);
        throw TerminalException(msg);
    }

    utils::setsid();
    utils::ioctl(ttyFD, TIOCSCTTY, NULL);

    utils::dup2(ttyFD, STDIN_FILENO);
    utils::dup2(ttyFD, STDOUT_FILENO);
    utils::dup2(ttyFD, STDERR_FILENO);
}

std::pair<int, std::string> openPty(bool rawMode)
{
    int master, slave;
    std::string ptsName;

    try {
        lxcpp::openpty(&master, &slave);

        utils::setCloseOnExec(master, true);
        utils::setNonBlocking(master, true);

        if (rawMode) {
            makeRawTerm(slave);
        }

        ptsName = lxcpp::ttyname_r(slave);

    } catch(std::exception&) {
        TEMP_FAILURE_RETRY(::close(master));
        TEMP_FAILURE_RETRY(::close(slave));

        throw;
    }

    utils::close(slave);
    return std::pair<int, std::string>(master, ptsName);
}

std::pair<int, std::string> openPty(const std::string &ptmx)
{
    int ptyNo;
    int unlock = 0;

    int master = utils::open(ptmx, O_RDWR|O_NOCTTY|O_NONBLOCK|O_CLOEXEC);
    utils::ioctl(master, TIOCSPTLCK, &unlock);
    utils::ioctl(master, TIOCGPTN, &ptyNo);

    std::string ptsName = std::to_string(ptyNo);

    return std::pair<int, std::string>(master, ptsName);
}


} // namespace lxcpp
