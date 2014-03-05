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
 * @file    ut-scs-log.cpp
 * @author  Lukasz Kostyra (l.kostyra@samsung.com)
 * @brief   Unit tests for security-containers logging system
 */

#include "ut.hpp"
#include "scs-log.hpp"

BOOST_AUTO_TEST_SUITE(LogSuite)

BOOST_AUTO_TEST_CASE(DumpAllLogTypes)
{
    LOGE("Logging an error message.");
    LOGW("Logging a warning.");
    LOGI("Logging some information.");
    LOGD("Logging debug information.");
    LOGT("Logging trace information.");
}

BOOST_AUTO_TEST_SUITE_END()
