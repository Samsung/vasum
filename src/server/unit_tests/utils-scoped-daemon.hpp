/*
 *  Copyright (c) 2000 - 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Bumjin Im <bj.im@samsung.com>
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
 * @file    utils-scoped-daemon.hpp
 * @author  Piotr Bartosiewicz (p.bartosiewi@partner.samsung.com)
 * @brief   Starts external daemon in constructor, stops it in destructor
 */

#ifndef UTILS_SCOPED_DAEMON_HPP
#define UTILS_SCOPED_DAEMON_HPP

#include <sys/types.h>

namespace security_containers {
namespace utils {

/**
 * External daemon launcher helper.
 */
class ScopedDaemon {
public:
    /**
     * Starts a daemon.
     * @param path daemon path
     * @param argv arguments passed to the daemon
     * @param useLauncher use additional launcher process
     */
    ScopedDaemon(const char* path, const char* const argv[], const bool useLauncher = true);

    /**
     * Stops a daemon if it is not stopped already.
     */
    ~ScopedDaemon();

    /**
     * Stops a daemon by sending SIGTERM and waits for a process.
     */
    void stop();
private:
    pid_t mPid;
};

} // namespace utils
} // namespace security_containers

#endif // UTILS_SCOPED_DAEMON_HPP
