/*
 *  Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Dariusz Michaluk <d.michaluk@samsung.com>
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License
 */

/**
 * @file
 * @author  Dariusz Michaluk <d.michaluk@samsung.com>
 * @brief   Run a process as a daemon
 */

#include "config.hpp"

#include "fd-utils.hpp"

#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>

namespace utils {

bool daemonize()
{
    pid_t pid = ::fork();

    if (pid == -1) {
        std::cerr << "Fork failed" << std::endl;
        return false;
    } else if (pid != 0) {
        exit (0);
    }

    if (::setsid() == -1) {
        return false;
    }

    pid = ::fork();

    /*
     * Fork a second child and exit immediately to prevent zombies.
     * This causes the second child process to be orphaned, making the init
     * process responsible for its cleanup. And, since the first child is
     * a session leader without a controlling terminal, it's possible for
     * it to acquire one by opening a terminal in the future (System V
     * based systems). This second fork guarantees that the child is no
     * longer a session leader, preventing the daemon from ever acquiring
     * a controlling terminal.
     */

    if (pid == -1) {
        std::cerr << "Fork failed" << std::endl;
        return false;
    } else if (pid != 0) {
        exit(0);
    }

    if (::chdir("/") == -1) {
        return false;
    }

    ::umask(0);

    /** Close all open standard file descriptors */
    int fd = ::open("/dev/null", O_RDWR);
    if (fd == -1) {
        return false;
    }

    for (int i = 0; i <= 2; i++) {
        if (::dup2(fd, i) == -1) {
            utils::close(fd);
            return false;
        }
    }
    utils::close(fd);

    return true;
}

} // namespace utils
