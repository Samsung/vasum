/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Jan Olszak <j.olszak@samsung.com>
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
 * @author  Jan Olszak (j.olszak@samsung.com)
 * @brief   Path utility functions declaration
 */

#ifndef COMMON_UTILS_PATHS_HPP
#define COMMON_UTILS_PATHS_HPP

#include <string>
#include <vector>
#include <algorithm>


namespace utils {


template <class ...Paths> std::string createFilePath(const Paths& ... paths)
{
    std::vector<std::string> pathVec = {paths...};
    std::string retPath = "";

    if (pathVec.empty()) {
        return retPath;
    }

    for (std::string& p : pathVec) {
        // Repeat until retPath is not empty
        if (retPath.empty() || p.empty()) {
            retPath += p;
            continue;
        }

        // We need a slash
        if (retPath.back() != '/' && p.front() != '/' && p.front() != '.') {
            retPath += "/" + p;
            continue;
        }

        // Too many slashes
        if (retPath.back() == '/' && p.front() == '/') {
            retPath += p.substr(1);
            continue;
        }

        retPath += p;
    }

    return retPath;
}

namespace {

inline void removeDuplicateSlashes(std::string& path)
{
    auto it = std::unique(path.begin(), path.end(),
                          [](char a, char b) {
                              return (a == '/' && a == b);
                          });
    path.erase(it, path.end());
}

inline void removeTrailingSlash(std::string& path)
{
    size_t size = path.size();

    if (size > 1 && path[size - 1] == '/') {
        path.resize(size - 1);
    }
}

} // namespace

/*
 * Gets the dir name of a file path, analogous to dirname(1)
 */
inline std::string dirName(std::string path)
{
    removeDuplicateSlashes(path);
    removeTrailingSlash(path);
    path.erase(std::find(path.rbegin(), path.rend(), '/').base(), path.end());
    removeTrailingSlash(path);

    if (path.empty()) {
        return ".";
    }

    return path;
}

/*
 * Gets absolute path to specified file (if needed)
 */
inline std::string getAbsolutePath(const std::string& path, const std::string& base)
{
    if (path[0] == '/') {
        return path;
    } else {
        return utils::createFilePath(base, "/", path);
    }
}

} // namespace utils


#endif // COMMON_UTILS_PATHS_HPP
