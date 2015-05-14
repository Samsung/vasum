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
 * @brief   Create directory in constructor, delete it in destructor
 */

#include "config.hpp"

#include "utils/scoped-dir.hpp"

#include <boost/filesystem.hpp>


namespace utils {

namespace fs = boost::filesystem;

ScopedDir::ScopedDir()
{
}

ScopedDir::ScopedDir(const std::string& path)
{
    create(path);
}

ScopedDir::~ScopedDir()
{
    remove();
}

void ScopedDir::create(const std::string& path)
{
    remove();
    if (!path.empty()) {
        mPath = path;
        fs::remove_all(path);
        fs::create_directories(path);
    }
}

void ScopedDir::remove()
{
    if (!mPath.empty()) {
        fs::remove_all(mPath);
        mPath.clear();
    }
}

} // namespace utils
