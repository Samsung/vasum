/*
 *  Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Maciej Karpiuk (m.karpiuk2@samsung.com)
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
 * @author Maciej Karpiuk (m.karpiuk2@samsung.com)
 * @brief  Pre-defined validation check functions
*/

#include "config.hpp"

#include "cargo-validator/validator.hpp"
#include "utils/fs.hpp"
#include "utils/exception.hpp"

using namespace utils;

bool cargo::validator::isNonEmptyString(const std::string &s)
{
    return !s.empty();
}

bool cargo::validator::isAbsolutePath(const std::string &s)
{
    return utils::isAbsolute(s);
}

bool cargo::validator::isFilePresent(const std::string &s)
{
    return utils::isRegularFile(s);
}

bool cargo::validator::isDirectoryPresent(const std::string &s)
{
    return utils::isDir(s);
}
