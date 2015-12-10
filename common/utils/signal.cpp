/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Jan Olszak <j.olszak@samsung.com>
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
 * @author  Jan Olszak (j.olszak@samsung.com)
 * @brief   Signal related functions
 */

#include "config.hpp"

#include "utils/signal.hpp"
#include "utils/exception.hpp"
#include "logger/logger.hpp"

#include <string>
#include <cerrno>
#include <cstring>
#include <csignal>

namespace utils {

namespace {

void setSignalMask(int how, const ::sigset_t& set)
{
    int ret = ::pthread_sigmask(how, &set, nullptr /*&oldSet*/);
    if(ret != 0) {
        const std::string msg = "Error in pthread_sigmask: " + getSystemErrorMessage(ret);
        LOGE(msg);
        throw UtilsException(msg);
    }
}

void changeSignal(int how, const int sigNum) {
    ::sigset_t set;
    if(-1 == ::sigemptyset(&set)) {
        const std::string msg = "Error in sigemptyset: " + getSystemErrorMessage();
        LOGE(msg);
        throw UtilsException(msg);
    }

    if(-1 ==::sigaddset(&set, sigNum)) {
        const std::string msg = "Error in sigaddset: " + getSystemErrorMessage();
        LOGE(msg);
        throw UtilsException(msg);
    }

    setSignalMask(how, set);
}

}// namespace

::sigset_t getSignalMask()
{
    ::sigset_t set;
    int ret = ::pthread_sigmask(0 /*ignored*/, nullptr /*get the oldset*/, &set);
    if(ret != 0) {
        const std::string msg = "Error in pthread_sigmask: " + getSystemErrorMessage(ret);
        LOGE(msg);
        throw UtilsException(msg);
    }
    return set;
}

bool isSignalPending(const int sigNum)
{
    ::sigset_t set;
    if (::sigpending(&set) == -1) {
        const std::string msg = "Error in sigpending: " + getSystemErrorMessage();
        LOGE(msg);
        throw UtilsException(msg);
    }

    int ret = ::sigismember(&set, sigNum);
    if (ret == -1) {
        const std::string msg = "Error in sigismember: " + getSystemErrorMessage();
        LOGE(msg);
        throw UtilsException(msg);
    }

    return ret;
}

bool waitForSignal(const int sigNum, int timeoutMs)
{
    int timeoutS = timeoutMs / 1000;
    timeoutMs -= timeoutS * 1000;

    struct ::timespec timeout = {
        timeoutS,
        timeoutMs * 1000000
    };

    ::sigset_t set;
    if(-1 == ::sigemptyset(&set)) {
        const std::string msg = "Error in sigemptyset: " + getSystemErrorMessage();
        LOGE(msg);
        throw UtilsException(msg);
    }

    if(-1 ==::sigaddset(&set, sigNum)) {
        const std::string msg = "Error in sigaddset: " + getSystemErrorMessage();
        LOGE(msg);
        throw UtilsException(msg);
    }

    ::siginfo_t info;
    if (::sigtimedwait(&set, &info, &timeout) == -1) {
        if (errno == EAGAIN) {
            return false;
        }

        const std::string msg = "Error in sigtimedwait: " + getSystemErrorMessage();
        LOGE(msg);
        throw UtilsException(msg);
    }

    return true;
}

bool isSignalBlocked(const int sigNum)
{
    ::sigset_t set = getSignalMask();

    int ret = ::sigismember(&set, sigNum);
    if(-1 == ret) {
        const std::string msg = "Error in sigismember: " + getSystemErrorMessage();
        LOGE(msg);
        throw UtilsException(msg);
    }

    return ret == 1;
}

void signalBlock(const int sigNum)
{
    changeSignal(SIG_BLOCK, sigNum);
}

void signalBlockAllExcept(const std::initializer_list<int>& signals)
{
    ::sigset_t set;
    if(-1 == ::sigfillset(&set)) {
        const std::string msg = "Error in sigfillset: " + getSystemErrorMessage();
        LOGE(msg);
        throw UtilsException(msg);
    }

    for(const int s: signals) {
        if(-1 == ::sigaddset(&set, s)) {
            const std::string msg = "Error in sigaddset: " + getSystemErrorMessage();
            LOGE(msg);
            throw UtilsException(msg);
        }
    }
    setSignalMask(SIG_BLOCK, set);
}

void signalUnblock(const int sigNum)
{
    changeSignal(SIG_UNBLOCK, sigNum);
}

std::vector<std::pair<int, struct ::sigaction>> signalIgnore(const std::initializer_list<int>& signals)
{
    struct ::sigaction act;
    struct ::sigaction old;
    act.sa_handler = SIG_IGN;
    std::vector<std::pair<int, struct ::sigaction>> oldAct;

    for(const int s: signals) {
        if(-1 == ::sigaction(s, &act, &old)) {
            const std::string msg = "Error in sigaction: " + getSystemErrorMessage();
            LOGE(msg);
            throw UtilsException(msg);
        }
        oldAct.emplace_back(s, old);
    }

    return oldAct;
}

struct ::sigaction signalSet(const int sigNum, const struct ::sigaction *sigAct)
{
    struct ::sigaction old;

    if(-1 == ::sigaction(sigNum, sigAct, &old)) {
        const std::string msg = "Error in sigaction: " + getSystemErrorMessage();
        LOGE(msg);
        throw UtilsException(msg);
    }

    return old;
}

void sendSignal(const pid_t pid, const int sigNum)
{
    if (-1 == ::kill(pid, sigNum)) {
        const std::string msg = "Error during killing pid: " + std::to_string(pid) +
                                " sigNum: " + std::to_string(sigNum) +
                                ": "  + getSystemErrorMessage();
        LOGE(msg);
        throw UtilsException(msg);
    }
}

} // namespace utils
