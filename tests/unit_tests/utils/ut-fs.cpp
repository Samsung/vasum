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

#include "config.hpp"
#include "ut.hpp"

#include "utils/fs.hpp"
#include "utils/exception.hpp"

#include <memory>
#include <sys/mount.h>
#include <boost/filesystem.hpp>

BOOST_AUTO_TEST_SUITE(UtilsFSSuite)

using namespace security_containers;
using namespace security_containers::utils;

namespace {

const std::string FILE_PATH = SC_TEST_CONFIG_INSTALL_DIR "/utils/ut-fs/file.txt";
const std::string FILE_CONTENT = "File content\n"
                                 "Line 1\n"
                                 "Line 2\n";
const std::string FILE_CONTENT_2 = "Some other content\n"
                                   "Just to see if\n"
                                   "everything is copied correctly\n";
const std::string FILE_CONTENT_3 = "More content\n"
                                   "More and more content\n"
                                   "That's a lot of data to test\n";
const std::string BUGGY_FILE_PATH = "/some/missing/file/path/file.txt";
const std::string TMP_PATH = "/tmp";
const std::string FILE_PATH_RANDOM =
    boost::filesystem::unique_path("/tmp/testFile-%%%%").string();
const std::string MOUNT_POINT_RANDOM_1 =
    boost::filesystem::unique_path("/tmp/mountPoint-%%%%").string();
const std::string MOUNT_POINT_RANDOM_2 =
    boost::filesystem::unique_path("/tmp/mountPoint-%%%%").string();
const std::string FILE_DIR_RANDOM_1 =
    boost::filesystem::unique_path("testDir-%%%%").string();
const std::string FILE_DIR_RANDOM_2 =
    boost::filesystem::unique_path("testDir-%%%%").string();
const std::string FILE_DIR_RANDOM_3 =
    boost::filesystem::unique_path("testDir-%%%%").string();
const std::string FILE_NAME_RANDOM_1 =
    boost::filesystem::unique_path("testFile-%%%%").string();
const std::string FILE_NAME_RANDOM_2 =
    boost::filesystem::unique_path("testFile-%%%%").string();

} // namespace

BOOST_AUTO_TEST_CASE(ReadFileContentTest)
{
    BOOST_CHECK_EQUAL(FILE_CONTENT, readFileContent(FILE_PATH));
    BOOST_CHECK_THROW(readFileContent(BUGGY_FILE_PATH), UtilsException);
}

BOOST_AUTO_TEST_CASE(SaveFileContentTest)
{
    BOOST_REQUIRE(saveFileContent(FILE_PATH_RANDOM, FILE_CONTENT));
    BOOST_CHECK_EQUAL(FILE_CONTENT, readFileContent(FILE_PATH));

    boost::system::error_code ec;
    boost::filesystem::remove(FILE_PATH_RANDOM, ec);
}

BOOST_AUTO_TEST_CASE(MountPointTest)
{
    bool result;
    namespace fs = boost::filesystem;
    boost::system::error_code ec;

    BOOST_REQUIRE(fs::create_directory(MOUNT_POINT_RANDOM_1, ec));
    BOOST_REQUIRE(isMountPoint(MOUNT_POINT_RANDOM_1, result));
    BOOST_CHECK_EQUAL(result, false);
    BOOST_REQUIRE(hasSameMountPoint(TMP_PATH, MOUNT_POINT_RANDOM_1, result));
    BOOST_CHECK_EQUAL(result, true);

    BOOST_REQUIRE(mountRun(MOUNT_POINT_RANDOM_1));
    BOOST_REQUIRE(isMountPoint(MOUNT_POINT_RANDOM_1, result));
    BOOST_CHECK_EQUAL(result, true);
    BOOST_REQUIRE(hasSameMountPoint(TMP_PATH, MOUNT_POINT_RANDOM_1, result));
    BOOST_CHECK_EQUAL(result, false);

    BOOST_REQUIRE(umount(MOUNT_POINT_RANDOM_1));
    BOOST_REQUIRE(fs::remove(MOUNT_POINT_RANDOM_1, ec));
}

BOOST_AUTO_TEST_CASE(MoveFileTest)
{
    namespace fs = boost::filesystem;
    boost::system::error_code ec;
    std::string src, dst;

    // same mount point
    src = TMP_PATH + "/" + FILE_NAME_RANDOM_1;
    dst = TMP_PATH + "/" + FILE_NAME_RANDOM_2;

    BOOST_REQUIRE(saveFileContent(src, FILE_CONTENT));

    BOOST_CHECK(moveFile(src, dst));
    BOOST_CHECK(!fs::exists(src));
    BOOST_CHECK_EQUAL(readFileContent(dst), FILE_CONTENT);

    BOOST_REQUIRE(fs::remove(dst));

    // different mount point
    src = TMP_PATH + "/" + FILE_NAME_RANDOM_1;
    dst = MOUNT_POINT_RANDOM_2 + "/" + FILE_NAME_RANDOM_2;

    BOOST_REQUIRE(fs::create_directory(MOUNT_POINT_RANDOM_2, ec));
    BOOST_REQUIRE(mountRun(MOUNT_POINT_RANDOM_2));
    BOOST_REQUIRE(saveFileContent(src, FILE_CONTENT));

    BOOST_CHECK(moveFile(src, dst));
    BOOST_CHECK(!fs::exists(src));
    BOOST_CHECK_EQUAL(readFileContent(dst), FILE_CONTENT);

    BOOST_REQUIRE(fs::remove(dst));
    BOOST_REQUIRE(umount(MOUNT_POINT_RANDOM_2));
    BOOST_REQUIRE(fs::remove(MOUNT_POINT_RANDOM_2, ec));
}

BOOST_AUTO_TEST_CASE(CopyDirContentsTest)
{
    namespace fs = boost::filesystem;
    std::string src, src_inner, dst, dst_inner;
    boost::system::error_code ec;

    src = TMP_PATH + "/" + FILE_DIR_RANDOM_1;
    src_inner = src + "/" + FILE_DIR_RANDOM_3;

    dst = TMP_PATH + "/" + FILE_DIR_RANDOM_2;
    dst_inner = dst + "/" + FILE_DIR_RANDOM_3;

    // create entire structure with files
    BOOST_REQUIRE(fs::create_directory(src, ec));
    BOOST_REQUIRE(ec.value() == 0);
    BOOST_REQUIRE(fs::create_directory(src_inner, ec));
    BOOST_REQUIRE(ec.value() == 0);

    BOOST_REQUIRE(saveFileContent(src + "/" + FILE_NAME_RANDOM_1, FILE_CONTENT));
    BOOST_REQUIRE(saveFileContent(src + "/" + FILE_NAME_RANDOM_2, FILE_CONTENT_2));
    BOOST_REQUIRE(saveFileContent(src_inner + "/" + FILE_NAME_RANDOM_1, FILE_CONTENT_3));

    BOOST_REQUIRE(fs::create_directory(dst, ec));
    BOOST_REQUIRE(ec.value() == 0);

    // copy data
    BOOST_CHECK(copyDirContents(src, dst));

    // check if copy is successful
    BOOST_CHECK(fs::exists(dst + "/" + FILE_NAME_RANDOM_1));
    BOOST_CHECK(fs::exists(dst + "/" + FILE_NAME_RANDOM_2));
    BOOST_CHECK(fs::exists(dst_inner));
    BOOST_CHECK(fs::exists(dst_inner + "/" + FILE_NAME_RANDOM_1));

    BOOST_CHECK_EQUAL(readFileContent(dst + "/" + FILE_NAME_RANDOM_1), FILE_CONTENT);
    BOOST_CHECK_EQUAL(readFileContent(dst + "/" + FILE_NAME_RANDOM_2), FILE_CONTENT_2);
    BOOST_CHECK_EQUAL(readFileContent(dst_inner + "/" + FILE_NAME_RANDOM_1), FILE_CONTENT_3);
}

BOOST_AUTO_TEST_SUITE_END()
