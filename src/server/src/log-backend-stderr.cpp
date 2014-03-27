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
 * @file    log-backend-stderr.cpp
 * @author  Pawel Broda (p.broda@partner.samsung.com)
 * @brief   Stderr backend for logger
 */


#include "log-backend-stderr.hpp"

#include <stdio.h>

namespace security_containers {
namespace log {

void StderrBackend::log(const std::string& message)
{
    fprintf(stderr, "%s", message.c_str());
}

} // namespace log
} // namespace security_containers
