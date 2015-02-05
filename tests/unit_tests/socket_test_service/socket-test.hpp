/*
 *  Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Lukasz Kostyra <l.kostyra@samsung.com>
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
 * @author  Lukasz Kostyra (l.kostyra@samsung.com)
 * @brief   Mini-service for IPC Socket mechanism tests
 */

#ifndef TESTS_SOCKET_TEST_SERVICE_HPP
#define TESTS_SOCKET_TEST_SERVICE_HPP

#include <string>

namespace vasum {
namespace socket_test {

const std::string SOCKET_PATH = "/run/vasum-socket-test.socket";
const std::string TEST_MESSAGE = "Some great messages, ey!";

} // namespace socket_test
} // namespace vasum

#endif // TESTS_SOCKET_TEST_SERVICE_HPP
