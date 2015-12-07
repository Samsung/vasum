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

#include "utils/exception.hpp"
#include "utils/execute.hpp"
#include "logger/logger.hpp"

#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

namespace utils {

namespace {

const uid_t UNSPEC_UID = static_cast<uid_t>(-1);


__attribute__((unused)) std::ostream& operator<< (std::ostream& out, const char* const* argv)
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

    pid_t pid = ::fork();
    if (pid == -1) {
        LOGE("Fork failed: " << getSystemErrorMessage());
        return false;
    }
    if (pid == 0) {
        func();
        ::_exit(EXIT_SUCCESS);
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

bool executeAndWait(const uid_t uid, const char* fname, const char* const* argv, int& status)
{
    LOGD("Execute " << fname << argv);

    pid_t pid = ::fork();
    if (pid == -1) {
        LOGE("Fork failed: " << getSystemErrorMessage());
        return false;
    }
    if (pid == 0) {
        if (uid != UNSPEC_UID && ::setuid(uid) < 0) {
            LOGW("Failed to become uid(" << uid << "): " << getSystemErrorMessage());
            ::_exit(EXIT_FAILURE);
        }
        ::execv(fname, const_cast<char* const*>(argv));
        LOGE("execv failed: " << getSystemErrorMessage());
        ::_exit(EXIT_FAILURE);
    }
    return waitPid(pid, status);
}

bool executeAndWait(const char* fname, const char* const* argv, int& status)
{
    return executeAndWait(-1, fname, argv, status);
}

bool executeAndWait(const char* fname, const char* const* argv)
{
    int status;
    if (!executeAndWait(UNSPEC_UID, fname, argv, status)) {
        return false;
    }
    return isExecutionSuccessful(status);
}

bool waitPid(pid_t pid, int& status)
{
    while (::waitpid(pid, &status, 0) == -1) {
        if (errno != EINTR) {
            LOGE("waitpid() failed: " << getSystemErrorMessage());
            return false;
        }
    }
    return true;
}

bool executeAndWait(const uid_t uid, const std::vector<std::string>& argv, int& status)
{
    std::vector<const char *> args;
    args.reserve(argv.size() + 1);

    for (const auto& arg : argv) {
        args.push_back(arg.c_str());
    }
    args.push_back(nullptr);

    return executeAndWait(uid, args[0], args.data(), status);
}

bool executeAndWait(const uid_t uid, const std::vector<std::string>& argv)
{
    int status;
    if (!executeAndWait(uid, argv, status)) {
        return false;
    }
    return isExecutionSuccessful(status);
}

bool executeAndWait(const std::vector<std::string>& argv)
{
    return executeAndWait(UNSPEC_UID, argv);
}

} // namespace utils
