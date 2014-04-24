/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Lukasz Pawelczyk <l.pawelczyk@partner.samsung.com>
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
 * @author  Lukasz Pawelczyk (l.pawelczyk@partner.samsung.com)
 * @brief   Unit tests of utils
 */

#include "ut.hpp"

#include "utils/fs.hpp"
#include "utils/exception.hpp"

#include <memory>

BOOST_AUTO_TEST_SUITE(UtilsFSSuite)

using namespace security_containers;
using namespace security_containers::utils;

const std::string FILE_PATH = SC_TEST_CONFIG_INSTALL_DIR "/utils/ut-fs/file.txt";
const std::string FILE_CONTENT = "File content\n"
                                 "Line 1\n"
                                 "Line 2\n";
const std::string BUGGY_FILE_PATH = "/some/missing/file/path/file.txt";

BOOST_AUTO_TEST_CASE(ReadFileContentTest)
{
    BOOST_CHECK_EQUAL(FILE_CONTENT, readFileContent(FILE_PATH));
    BOOST_CHECK_THROW(readFileContent(BUGGY_FILE_PATH), UtilsException);
}

BOOST_AUTO_TEST_SUITE_END()
