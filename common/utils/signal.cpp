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
        const std::string msg = getSystemErrorMessage(ret);
        LOGE("Error in pthread_sigmask: " << msg);
        throw UtilsException("Error in pthread_sigmask: " + msg);
    }
}

void changeSignal(int how, const int sigNum) {
    ::sigset_t set;
    if(-1 == ::sigemptyset(&set)) {
        const std::string msg = getSystemErrorMessage();
        LOGE("Error in sigfillset: " << msg);
        throw UtilsException("Error in sigfillset: " + msg);
    }

    if(-1 ==::sigaddset(&set, sigNum)) {
        const std::string msg = getSystemErrorMessage();
        LOGE("Error in sigdelset: " << msg);
        throw UtilsException("Error in sigdelset: " + msg);
    }

    setSignalMask(how, set);
}

}// namespace

::sigset_t getSignalMask()
{
    ::sigset_t set;
    int ret = ::pthread_sigmask(0 /*ignored*/, nullptr /*get the oldset*/, &set);
    if(ret != 0) {
        const std::string msg = getSystemErrorMessage(ret);
        LOGE("Error in pthread_sigmask: " << msg);
        throw UtilsException("Error in pthread_sigmask: " + msg);
    }
    return set;
}

bool isSignalBlocked(const int sigNum)
{
    ::sigset_t set = getSignalMask();

    int ret = ::sigismember(&set, sigNum);
    if(-1 == ret) {
        const std::string msg = getSystemErrorMessage();
        LOGE("Error in sigismember: " << msg);
        throw UtilsException("Error in sigismember: " + msg);
    }

    return ret == 1;
}

void signalBlock(const int sigNum)
{
    changeSignal(SIG_BLOCK, sigNum);
}

void signalBlockAll()
{
    ::sigset_t set;
    if(-1 == ::sigfillset(&set)) {
        const std::string msg = getSystemErrorMessage();
        LOGE("Error in sigfillset: " << msg);
        throw UtilsException("Error in sigfillset: " + msg);
    }

    setSignalMask(SIG_BLOCK, set);
}

void signalUnblock(const int sigNum)
{
    changeSignal(SIG_UNBLOCK, sigNum);
}

} // namespace utils





