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
#include "utils/scoped-dir.hpp"

#include <memory>
#include <sys/mount.h>
#include <boost/filesystem.hpp>

using namespace utils;

namespace {

const std::string TEST_PATH = "/tmp/ut-fsutils";
const std::string REFERENCE_FILE_PATH = VSM_TEST_CONFIG_INSTALL_DIR "/utils/file.txt";
const std::string REFERENCE_FILE_CONTENT = "File content\n"
                                           "Line 1\n"
                                           "Line 2\n";
const std::string FILE_CONTENT_2 = "Some other content\n"
                                   "Just to see if\n"
                                   "everything is copied correctly\n";
const std::string FILE_CONTENT_3 = "More content\n"
                                   "More and more content\n"
                                   "That's a lot of data to test\n";
const std::string BUGGY_FILE_PATH = TEST_PATH + "/missing/file.txt";
const std::string FILE_PATH = TEST_PATH + "/testFile";
const std::string MOUNT_POINT_1 = TEST_PATH + "/mountPoint-1";
const std::string MOUNT_POINT_2 = TEST_PATH + "/mountPoint-2";
const std::string FILE_DIR_1 = "testDir-1";
const std::string FILE_DIR_2 = "testDir-2";
const std::string FILE_DIR_3 = "testDir-3";
const std::string FILE_DIR_4 = "testDir-4";
const std::string FILE_NAME_1 = "testFile-1";
const std::string FILE_NAME_2 = "testFile-2";

struct Fixture {
    utils::ScopedDir mTestPathGuard;
    Fixture()
        : mTestPathGuard(TEST_PATH)
    {}
};

} // namespace

BOOST_FIXTURE_TEST_SUITE(UtilsFSSuite, Fixture)

BOOST_AUTO_TEST_CASE(ReadFileContent)
{
    BOOST_CHECK_EQUAL(REFERENCE_FILE_CONTENT, readFileContent(REFERENCE_FILE_PATH));
    BOOST_CHECK_EXCEPTION(readFileContent(BUGGY_FILE_PATH),
                          UtilsException,
                          WhatEquals("Read failed"));
}

BOOST_AUTO_TEST_CASE(SaveFileContent)
{
    BOOST_REQUIRE(saveFileContent(FILE_PATH, REFERENCE_FILE_CONTENT));
    BOOST_CHECK_EQUAL(REFERENCE_FILE_CONTENT, readFileContent(FILE_PATH));
}

BOOST_AUTO_TEST_CASE(RemoveFile)
{
    BOOST_REQUIRE(saveFileContent(FILE_PATH, REFERENCE_FILE_CONTENT));
    BOOST_REQUIRE(removeFile(FILE_PATH));
    BOOST_REQUIRE(!boost::filesystem::exists(FILE_PATH));
}

BOOST_AUTO_TEST_CASE(MountPoint)
{
    bool result;
    namespace fs = boost::filesystem;
    boost::system::error_code ec;

    BOOST_REQUIRE(fs::create_directory(MOUNT_POINT_1, ec));
    BOOST_REQUIRE(isMountPoint(MOUNT_POINT_1, result));
    BOOST_CHECK_EQUAL(result, false);
    BOOST_REQUIRE(hasSameMountPoint(TEST_PATH, MOUNT_POINT_1, result));
    BOOST_CHECK_EQUAL(result, true);

    BOOST_REQUIRE(mountRun(MOUNT_POINT_1));
    BOOST_REQUIRE(isMountPoint(MOUNT_POINT_1, result));
    BOOST_CHECK_EQUAL(result, true);
    BOOST_REQUIRE(hasSameMountPoint(TEST_PATH, MOUNT_POINT_1, result));
    BOOST_CHECK_EQUAL(result, false);

    BOOST_REQUIRE(umount(MOUNT_POINT_1));
    BOOST_REQUIRE(fs::remove(MOUNT_POINT_1, ec));
}

BOOST_AUTO_TEST_CASE(MoveFile)
{
    namespace fs = boost::filesystem;
    boost::system::error_code ec;
    std::string src, dst;

    // same mount point
    src = TEST_PATH + "/" + FILE_NAME_1;
    dst = TEST_PATH + "/" + FILE_NAME_2;

    BOOST_REQUIRE(saveFileContent(src, REFERENCE_FILE_CONTENT));

    BOOST_CHECK(moveFile(src, dst));
    BOOST_CHECK(!fs::exists(src));
    BOOST_CHECK_EQUAL(readFileContent(dst), REFERENCE_FILE_CONTENT);

    BOOST_REQUIRE(fs::remove(dst));

    // different mount point
    src = TEST_PATH + "/" + FILE_NAME_1;
    dst = MOUNT_POINT_2 + "/" + FILE_NAME_2;

    BOOST_REQUIRE(fs::create_directory(MOUNT_POINT_2, ec));
    BOOST_REQUIRE(mountRun(MOUNT_POINT_2));
    BOOST_REQUIRE(saveFileContent(src, REFERENCE_FILE_CONTENT));

    BOOST_CHECK(moveFile(src, dst));
    BOOST_CHECK(!fs::exists(src));
    BOOST_CHECK_EQUAL(readFileContent(dst), REFERENCE_FILE_CONTENT);

    BOOST_REQUIRE(fs::remove(dst));
    BOOST_REQUIRE(umount(MOUNT_POINT_2));
    BOOST_REQUIRE(fs::remove(MOUNT_POINT_2, ec));
}

BOOST_AUTO_TEST_CASE(CopyDirContents)
{
    namespace fs = boost::filesystem;
    std::string src, src_inner, src_inner2, dst, dst_inner, dst_inner2;
    boost::system::error_code ec;

    src = TEST_PATH + "/" + FILE_DIR_1;
    src_inner = src + "/" + FILE_DIR_3;
    src_inner2 = src + "/" + FILE_DIR_4;

    dst = TEST_PATH + "/" + FILE_DIR_2;
    dst_inner = dst + "/" + FILE_DIR_3;
    dst_inner2 = dst + "/" + FILE_DIR_4;

    // template dir structure:
    // |-src
    //    |-FILE_NAME_1
    //    |-FILE_NAME_2
    //    |-src_inner (rw directory)
    //    |  |-FILE_NAME_1
    //    |
    //    |-src_inner2 (ro directory)
    //       |-FILE_NAME_1
    //       |-FILE_NAME_2

    // create entire structure with files
    BOOST_REQUIRE(fs::create_directory(src, ec));
    BOOST_REQUIRE(ec.value() == 0);
    BOOST_REQUIRE(fs::create_directory(src_inner, ec));
    BOOST_REQUIRE(ec.value() == 0);
    BOOST_REQUIRE(fs::create_directory(src_inner2, ec));
    BOOST_REQUIRE(ec.value() == 0);

    BOOST_REQUIRE(saveFileContent(src + "/" + FILE_NAME_1, REFERENCE_FILE_CONTENT));
    BOOST_REQUIRE(saveFileContent(src + "/" + FILE_NAME_2, FILE_CONTENT_2));
    BOOST_REQUIRE(saveFileContent(src_inner + "/" + FILE_NAME_1, FILE_CONTENT_3));
    BOOST_REQUIRE(saveFileContent(src_inner2 + "/" + FILE_NAME_1, FILE_CONTENT_3));
    BOOST_REQUIRE(saveFileContent(src_inner2 + "/" + FILE_NAME_2, FILE_CONTENT_2));

    // change permissions of src_inner2 directory
    fs::permissions(src_inner2, fs::owner_read, ec);
    BOOST_REQUIRE(ec.value() == 0);

    // create dst directory
    BOOST_REQUIRE(fs::create_directory(dst, ec));
    BOOST_REQUIRE(ec.value() == 0);

    // copy data
    BOOST_CHECK(copyDirContents(src, dst));

    // check if copy is successful
    BOOST_CHECK(fs::exists(dst + "/" + FILE_NAME_1));
    BOOST_CHECK(fs::exists(dst + "/" + FILE_NAME_2));
    BOOST_CHECK(fs::exists(dst_inner));
    BOOST_CHECK(fs::exists(dst_inner + "/" + FILE_NAME_1));
    BOOST_CHECK(fs::exists(dst_inner2));
    BOOST_CHECK(fs::exists(dst_inner2 + "/" + FILE_NAME_1));
    BOOST_CHECK(fs::exists(dst_inner2 + "/" + FILE_NAME_2));

    BOOST_CHECK_EQUAL(readFileContent(dst + "/" + FILE_NAME_1), REFERENCE_FILE_CONTENT);
    BOOST_CHECK_EQUAL(readFileContent(dst + "/" + FILE_NAME_2), FILE_CONTENT_2);
    BOOST_CHECK_EQUAL(readFileContent(dst_inner + "/" + FILE_NAME_1), FILE_CONTENT_3);
    BOOST_CHECK_EQUAL(readFileContent(dst_inner2 + "/" + FILE_NAME_1), FILE_CONTENT_3);
    BOOST_CHECK_EQUAL(readFileContent(dst_inner2 + "/" + FILE_NAME_2), FILE_CONTENT_2);

    fs::file_status st = fs::status(fs::path(dst_inner2));
    BOOST_CHECK(fs::owner_read == st.permissions());
}

BOOST_AUTO_TEST_SUITE_END()
