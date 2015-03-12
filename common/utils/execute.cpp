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
#include "base-exception.hpp"
#include "utils/execute.hpp"
#include "logger/logger.hpp"

#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

namespace vasum {
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

bool isExecutionSuccessful(int status)
{
    if (!WIFEXITED(status)) {
        if (WIFSIGNALED(status)) {
            LOGE("Child terminated by signal, signal: " << WTERMSIG(status));
        } else if (WIFSTOPPED(status)) {
            LOGW("Child was stopped by signal " << WSTOPSIG(status));
        } else {
            LOGE("Child exited abnormally, status: " << status);
        }
        return false;
    }
    if (WEXITSTATUS(status) != EXIT_SUCCESS) {
        LOGE("Child exit status: " << WEXITSTATUS(status));
        return false;
    }
    return true;
}

} // namespace

bool executeAndWait(const std::function<void()>& func, int& status)
{
    LOGD("Execute child process");

    pid_t pid = fork();
    if (pid == -1) {
        LOGE("Fork failed: " << vasum::getSystemErrorMessage());
        return false;
    }
    if (pid == 0) {
        func();
        _exit(EXIT_SUCCESS);
    }
    return waitPid(pid, status);
}

bool executeAndWait(const std::function<void()>& func)
{
    int status;
    if (!executeAndWait(func, status)) {
        return false;
    }
    return isExecutionSuccessful(status);
}


bool executeAndWait(const char* fname, const char* const* argv, int& status)
{
    LOGD("Execute " << fname << argv);

    bool success = executeAndWait([=]() {
        execv(fname, const_cast<char* const*>(argv));
        _exit(EXIT_FAILURE);
    }, status);
    if (!success) {
        LOGW("Process " << fname << " has exited abnormally");
    }
    return success;
}

bool executeAndWait(const char* fname, const char* const* argv)
{
    int status;
    if (!executeAndWait(fname, argv, status)) {
        return false;
    }
    return isExecutionSuccessful(status);
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
} // namespace vasum
