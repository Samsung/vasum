/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Lukasz Pawelczyk <l.pawelczyk@partner.samsung.com>
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
 * @author  Lukasz Pawelczyk (l.pawelczyk@partner.samsung.com)
 * @brief   File utility functions
 */

#include "log/logger.hpp"
#include "utils/fs.hpp"
#include "utils/exception.hpp"

#include <fstream>
#include <streambuf>


namespace security_containers {
namespace utils {


std::string readFileContent(const std::string& path)
{
    std::ifstream t(path);

    if (!t.is_open()) {
        LOGE(path << " is missing");
        throw UtilsException();
    }

    std::string content;

    t.seekg(0, std::ios::end);
    content.reserve(t.tellg());
    t.seekg(0, std::ios::beg);

    content.assign((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());

    return content;
}


} // namespace utils
} // namespace security_containers
