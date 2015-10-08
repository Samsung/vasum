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

#ifndef COMMON_UTILS_SIGNAL_HPP
#define COMMON_UTILS_SIGNAL_HPP

#include <initializer_list>
#include <csignal>
#include <vector>


namespace utils {


::sigset_t getSignalMask();
bool isSignalPending(const int sigNum);
bool waitForSignal(const int sigNum, int timeoutMs);
bool isSignalBlocked(const int sigNum);
void signalBlockAllExcept(const std::initializer_list<int>& signals);
void signalBlock(const int sigNum);
void signalUnblock(const int sigNum);
std::vector<std::pair<int, struct ::sigaction>> signalIgnore(const std::initializer_list<int>& signals);
struct ::sigaction signalSet(const int sigNum, const struct ::sigaction *sigAct);
void sendSignal(const pid_t pid, const int sigNum);


} // namespace utils


#endif // COMMON_UTILS_SIGNAL_HPP
