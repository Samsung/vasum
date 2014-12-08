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

#include "config/fields.hpp"
#include "config/fields-union.hpp"
#include <string>
#include <vector>


namespace vasum {

struct ZoneProvisioning
{

    struct File
    {
        std::int32_t type;
        std::string path;
        std::int32_t flags;
        std::int32_t mode;

        CONFIG_REGISTER
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

        CONFIG_REGISTER
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

        CONFIG_REGISTER
        (
            source,
            target
        )
    };

    struct Unit
    {
        CONFIG_DECLARE_UNION
        (
            File,
            Mount,
            Link
        )
    };

    std::vector<Unit> units;

    CONFIG_REGISTER
    (
        units
    )
};

} // namespace vasum

#endif // SERVER_PROVISIONING_CONFIG_HPP
