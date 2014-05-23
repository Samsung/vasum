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
 * @brief   Wait for file utility function
 */

#include "config.hpp"
#include "utils/exception.hpp"
#include "utils/file-wait.hpp"

#include <sys/stat.h>
#include <unistd.h>
#include <stdexcept>


namespace security_containers {
namespace utils {


const unsigned int GRANULARITY = 100;

void waitForFile(const std::string& filename, const unsigned int timeoutMs)
{
    //TODO this is a temporary solution, use inotify instead of sleep
    struct stat s;
    unsigned int loops = 0;
    while (stat(filename.c_str(), &s) == -1) {
        if (errno != ENOENT) {
            throw UtilsException("file access error: " + filename);
        }
        ++ loops;
        if (loops * GRANULARITY > timeoutMs) {
            throw UtilsException("timeout on waiting for: " + filename);
        }
        usleep(GRANULARITY * 1000);
    }
}


} // namespace utils
} // namespace security_containers
