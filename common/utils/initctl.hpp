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
 * @brief   Api for talking to init via initctl
 */

#ifndef COMMON_UTILS_INITCTL_HPP
#define COMMON_UTILS_INITCTL_HPP


namespace utils {

enum RunLevel : int {
    RUNLEVEL_POWEROFF = 0,
    RUNLEVEL_REBOOT = 6
};

bool setRunLevel(RunLevel runLevel);


} // namespace utils


#endif // COMMON_UTILS_INITCTL_HPP
