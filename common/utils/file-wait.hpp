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

#ifndef COMMON_UTILS_FILE_WAIT_HPP
#define COMMON_UTILS_FILE_WAIT_HPP

#include <string>


namespace utils {


//TODO It is used in unit tests now, but it is unclear
//     whether the same solution will be used in daemon.
void waitForFile(const std::string& filename, const unsigned int timeoutMs);


} // namespace utils


#endif // COMMON_UTILS_FILE_WAIT_HPP
