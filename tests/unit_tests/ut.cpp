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
 * @brief   Main file for the Vasum Daemon unit tests
 */

#include "config.hpp"

#include "logger/logger.hpp"
#include "logger/backend-stderr.hpp"

#include <boost/test/included/unit_test.hpp>


using namespace boost::unit_test;
using namespace logger;

test_suite* init_unit_test_suite(int /*argc*/, char** /*argv*/)
{
    Logger::setLogLevel(LogLevel::TRACE);
    Logger::setLogBackend(new StderrBackend());

    return NULL;
}
