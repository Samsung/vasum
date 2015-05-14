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

#ifndef UNIT_TESTS_UTILS_SCOPED_DIR_HPP
#define UNIT_TESTS_UTILS_SCOPED_DIR_HPP

#include <string>


namespace utils {


/**
 * Scoped directory
 * To be used in tests only
 */
class ScopedDir {
public:
    ScopedDir();
    ScopedDir(const std::string& path);
    ~ScopedDir();

    /**
     * Creates a dir or if exists ensures it is empty
     */
    void create(const std::string& path);

    /**
     * Deletes this dir with all content
     */
    void remove();

private:
    std::string mPath;
};


} // namespace utils


#endif // UNIT_TESTS_UTILS_SCOPED_DIR_HPP
