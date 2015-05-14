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
 * @brief   Starts external daemon in constructor, stops it in destructor
 */

#ifndef UNIT_TESTS_UTILS_SCOPED_DAEMON_HPP
#define UNIT_TESTS_UTILS_SCOPED_DAEMON_HPP

#include <sys/types.h>


namespace utils {


/**
 * External daemon launcher helper.
 */
class ScopedDaemon {
public:
    ScopedDaemon();

    /**
     * Stops a daemon if it is not stopped already.
     */
    ~ScopedDaemon();

    /**
     * Starts a daemon.
     * @param path daemon path
     * @param argv arguments passed to the daemon
     * @param useLauncher use additional launcher process
     */
    void start(const char* path, const char* const argv[], const bool useLauncher = false);

    /**
     * Stops a daemon by sending SIGTERM and waits for a process.
     */
    void stop();
private:
    pid_t mPid;
};


} // namespace utils


#endif // UNIT_TESTS_UTILS_SCOPED_DAEMON_HPP
