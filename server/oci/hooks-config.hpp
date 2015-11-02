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
 * @brief   Hooks configuration
 */

#ifndef HOOKS_CONFIG_HPP
#define HOOKS_CONFIG_HPP

#include "config.hpp"
#include "cargo/fields.hpp"

#include <string>
#include <vector>

namespace vasum {

struct HookConfig {

    /**
     * specifies the path to hook
     */
    std::string path;

    /**
     * specifies hook arguments
     */
    std::vector<std::string> args;

    /**
     * contains a list of variables that will be set in the process's environment
     */
    std::vector<std::string> env;

    CARGO_REGISTER
    (
        path,
        args,
        env
    )
};

/**
 * Hooks are a collection of actions to perform at various container lifecycle events.
 */
struct HooksConfig {

    /**
     * list of hooks to be run before the container process is executed
     */
    std::vector<HookConfig> prestart;

    /**
     * list of hooks to be run immediately after the container process is started
     */
    std::vector<HookConfig> poststart;

    /**
     * list of hooks to be run after the container process exits
     */
    std::vector<HookConfig> poststop;

    CARGO_REGISTER
    (
        prestart,
        poststart,
        poststop
    )
};

} // namespace vasum


#endif // HOOKS_CONFIG_HPP
