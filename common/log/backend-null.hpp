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
 * @file
 * @author  Pawel Broda (p.broda@partner.samsung.com)
 * @brief   Null backend for logger
 */


#ifndef COMMON_LOG_BACKEND_NULL_HPP
#define COMMON_LOG_BACKEND_NULL_HPP

#include "log/backend.hpp"


namespace security_containers {
namespace log {


/**
    Null logging backend
 */
class NullLogger : public LogBackend {
public:
    void log(const std::string& /*message*/) override {}
};


} // namespace log
} // namespace security_containers


#endif // COMMON_LOG_BACKEND_NULL_HPP
