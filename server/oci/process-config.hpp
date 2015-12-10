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
 * @author  Dariusz Michaluk (d.michaluk@samsung.com)
 * @brief   Process configuration
 */

#ifndef PROCESS_CONFIG_HPP
#define PROCESS_CONFIG_HPP

#include "cargo/fields.hpp"

#include <string>
#include <vector>

namespace vasum {

struct UserConfig {

    /**
     * specifies the user/group id
     */
    int uid;
    int gid;

    /**
     * specifies additional group ids to be added to the process
     */
    std::vector<int> additionalGids;

    CARGO_REGISTER
    (
        uid,
        gid,
        additionalGids
    )
};

struct ProcessConfig {

    /**
     * specifies whether you want a terminal attached to that process.
     */
    bool terminal;

    /**
     * which user the process runs as
     */
    UserConfig user;

    /**
     * executable to launch and any flags
     */
    std::vector<std::string> args;

    /**
     * contains a list of variables that will be set in the process's environment
     */
    std::vector<std::string> env;

    /**
     * working directory that will be set for the executable
     */
    std::string cwd;

    CARGO_REGISTER
    (
        terminal,
        user,
        args,
        env,
        cwd
    )
};

} // namespace vasum


#endif // PROCESS_CONFIG_HPP
