/*
 *  Copyright (c) 2000 - 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Bumjin Im <bj.im@samsung.com>
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
 * @file    log.hpp
 * @author  Jan Olszak (j.olszak@samsung.com)
 * @brief   Logging macros
 */


#ifndef LOG_HPP
#define LOG_HPP

#include <iostream>
#include <string.h>

#define BASE_FILE (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define LOG_LEVEL(level, ...) \
    std::cout << "[" << level << "] " << BASE_FILE << ":" << __LINE__ << " " \
              << __VA_ARGS__ << std::endl

#define LOGE(...) LOG_LEVEL("ERROR", __VA_ARGS__)
#define LOGW(...) LOG_LEVEL("WARN ", __VA_ARGS__)
#define LOGI(...) LOG_LEVEL("INFO ", __VA_ARGS__)
#define LOGD(...) LOG_LEVEL("DEBUG", __VA_ARGS__)
#define LOGT(...) LOG_LEVEL("TRACE", __VA_ARGS__)


#endif // LOG_HPP
