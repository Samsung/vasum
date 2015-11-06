/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Mateusz Malicki <m.malicki2@samsung.com>
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
 * @author  Mateusz Malicki (m.malicki2@samsung.com)
 * @brief   Declaration of the class for storing zone provisioning configuration
 */


#ifndef SERVER_PROVISIONING_CONFIG_HPP
#define SERVER_PROVISIONING_CONFIG_HPP

#include "cargo/fields.hpp"
#include "cargo/fields-union.hpp"
#include <string>
#include <vector>


namespace vasum {

struct ZoneProvisioningConfig
{

    struct File
    {
        std::int32_t type;
        std::string path;
        std::int32_t flags;
        std::int32_t mode;

        CARGO_REGISTER
        (
            type,
            path,
            flags,
            mode
        )
    };

    struct Mount
    {
        std::string source;
        std::string target;
        std::string type;
        std::int64_t flags;
        std::string data;

        CARGO_REGISTER
        (
            source,
            target,
            type,
            flags,
            data
        )
    };

    struct Link
    {
        std::string source;
        std::string target;

        CARGO_REGISTER
        (
            source,
            target
        )
    };

    struct Provision
    {
        CARGO_DECLARE_UNION
        (
            File,
            Mount,
            Link
        )
    };

    std::vector<Provision> provisions;

    CARGO_REGISTER
    (
        provisions
    )
};

} // namespace vasum

#endif // SERVER_PROVISIONING_CONFIG_HPP
