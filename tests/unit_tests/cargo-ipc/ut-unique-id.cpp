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
 * @brief   Unit tests of UID
 */

#include "config.hpp"

#include "ut.hpp"

#include "cargo-ipc/unique-id.hpp"

#include <string>
#include <sstream>
#include <uuid/uuid.h>

namespace {

const std::string EMPTY_UUID = "00000000-0000-0000-0000-000000000000";

} // namespace

BOOST_AUTO_TEST_SUITE(UniqueIDSuite)

// constructor should provide empty timestamp and UUID
BOOST_AUTO_TEST_CASE(Constructor)
{
    cargo::ipc::UniqueID uid;

    BOOST_CHECK_EQUAL(static_cast<std::int64_t>(uid.mTime.tv_sec), 0);
    BOOST_CHECK_EQUAL(uid.mTime.tv_nsec, 0);
    char uuid[37]; // 36 chars + terminating zero
    ::uuid_unparse(uid.mUUID, uuid);
    BOOST_CHECK(EMPTY_UUID.compare(uuid) == 0);
}

// generate one UID and compare with empty
BOOST_AUTO_TEST_CASE(Generate)
{
    cargo::ipc::UniqueID uid, emptyuid;
    uid.generate();

    BOOST_CHECK_NE(uid, emptyuid);
}

// generate two UIDs and compare them
BOOST_AUTO_TEST_CASE(DoubleGenerate)
{
    cargo::ipc::UniqueID uid1, uid2;

    uid1.generate();
    uid2.generate();
    BOOST_CHECK_NE(uid1, uid2);
}

// compare two empty UIDs
BOOST_AUTO_TEST_CASE(EmptyCompare)
{
    cargo::ipc::UniqueID uid1, uid2;

    BOOST_CHECK_EQUAL(uid1, uid2);
}

// pass empty UID to a stream
BOOST_AUTO_TEST_CASE(StreamOperator)
{
    cargo::ipc::UniqueID uid;
    std::stringstream ss;

    ss << uid;
    BOOST_CHECK_EQUAL(ss.str(), "0.0:" + EMPTY_UUID);
}

BOOST_AUTO_TEST_SUITE_END()
