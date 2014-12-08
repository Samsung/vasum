/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Piotr Bartosiewicz <p.bartosiewi@partner.samsung.com>
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
 * @author  Piotr Bartosiewicz (p.bartosiewi@partner.samsung.com)
 * @brief   Execute utils
 */

#include "config.hpp"
#include "utils/execute.hpp"
#include "logger/logger.hpp"

#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

namespace security_containers {
namespace utils {

namespace {

std::ostream& operator<< (std::ostream& out, const char* const* argv)
{
    if (*argv) {
        argv++; //skip
    }
    while (*argv) {
        out << " '" << *argv++ << "'";
    }
    return out;
}

} // namespace

bool executeAndWait(const char* fname, const char* const* argv, int& status)
{
    LOGD("Execute " << fname << argv);

    pid_t pid = fork();
    if (pid == -1) {
        LOGE("Fork failed");
        return false;
    }
    if (pid == 0) {
        execv(fname, const_cast<char* const*>(argv));
        _exit(EXIT_FAILURE);
    }
    return waitPid(pid, status);
}

bool executeAndWait(const char* fname, const char* const* argv)
{
    int status;
    if (!executeAndWait(fname, argv, status)) {
        return false;
    }
    if (status != EXIT_SUCCESS) {
        if (WIFEXITED(status)) {
            LOGW("Process " << fname << " has exited with status " << WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            LOGW("Process " << fname << " was killed by signal " << WTERMSIG(status));
        } else if (WIFSTOPPED(status)) {
            LOGW("Process " << fname << " was stopped by signal " << WSTOPSIG(status));
        }
        return false;
    }
    return true;
}

bool waitPid(pid_t pid, int& status)
{
    while (waitpid(pid, &status, 0) == -1) {
        if (errno != EINTR) {
            return false;
        }
    }
    return true;
}

} // namespace utils
} // namespace security_containers
