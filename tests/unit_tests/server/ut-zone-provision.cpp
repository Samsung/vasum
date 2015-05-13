/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Mateusz Malicki <m.malicki2@samsung.com>
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
 * @author  Mateusz Malicki (m.malicki2@samsung.com)
 * @brief   Unit tests of the ZoneProvision class
 */

#include "config.hpp"
#include "ut.hpp"

#include "utils/scoped-dir.hpp"
#include "utils/exception.hpp"
#include "utils/fs.hpp"
#include "config/manager.hpp"
#include "zone-provision.hpp"
#include "zone-provision-config.hpp"
#include "vasum-client.h"

#include <string>
#include <fstream>

#include <boost/filesystem.hpp>
#include <sys/mount.h>
#include <fcntl.h>

using namespace vasum;
using namespace utils;
using namespace config;

namespace fs = boost::filesystem;

namespace {

const std::string TEST_CONFIG_PATH = VSM_TEST_CONFIG_INSTALL_DIR "/provision/test-provision.conf";
const std::string ZONE = "ut-zone-provision-test";
const fs::path ZONES_PATH = "/tmp/ut-zones";
const fs::path ZONE_PATH = ZONES_PATH / fs::path(ZONE);
const fs::path SOME_FILE_PATH = ZONE_PATH / "file.txt";
const fs::path ROOTFS_PATH = ZONE_PATH / "rootfs";
const fs::path DB_PATH = ZONES_PATH / "vasum.db";
const std::string DB_PREFIX = "zone";

struct Fixture {
    utils::ScopedDir mZonesPathGuard;
    utils::ScopedDir mRootfsPath;

    Fixture()
        : mZonesPathGuard(ZONES_PATH.string())
        , mRootfsPath(ROOTFS_PATH.string())
    {
        BOOST_REQUIRE(utils::saveFileContent(SOME_FILE_PATH.string(), "text"));
    }

    ZoneProvision create(const std::vector<std::string>& validLinkPrefixes)
    {
        return ZoneProvision(ROOTFS_PATH.string(),
                             TEST_CONFIG_PATH,
                             DB_PATH.string(),
                             DB_PREFIX,
                             validLinkPrefixes);
    }

    void load(ZoneProvisioningConfig& config)
    {
        config::loadFromKVStoreWithJsonFile(DB_PATH.string(), TEST_CONFIG_PATH, config, DB_PREFIX);
    }

    void save(const ZoneProvisioningConfig& config)
    {
        config::saveToKVStore(DB_PATH.string(), config, DB_PREFIX);
    }
};

} // namespace


BOOST_FIXTURE_TEST_SUITE(ZoneProvisionSuite, Fixture)

BOOST_AUTO_TEST_CASE(Destructor)
{
    const fs::path mountTarget = fs::path("/opt/usr/data/ut-from-host-provision");
    const fs::path mountSource = fs::path("/tmp/ut-provision");
    {
        utils::ScopedDir provisionfs(mountSource.string());

        ZoneProvisioningConfig config;
        ZoneProvisioningConfig::Provision provision;
        provision.set(ZoneProvisioningConfig::File({VSMFILE_DIRECTORY,
                                        mountTarget.string(),
                                        0,
                                        0777}));
        config.provisions.push_back(provision);
        provision.set(ZoneProvisioningConfig::Mount({mountSource.string(),
                                          mountTarget.string(),
                                          "",
                                          MS_BIND,
                                          ""}));
        config.provisions.push_back(provision);
        save(config);

        ZoneProvision zoneProvision = create({});
        zoneProvision.start();
    }
    BOOST_CHECK(!fs::exists(mountSource));
}

BOOST_AUTO_TEST_CASE(File)
{
    //TODO: Test Fifo
    const fs::path regularFile = fs::path("/opt/usr/data/ut-regular-file");
    const fs::path copyFile = SOME_FILE_PATH;

    ZoneProvisioningConfig config;
    ZoneProvisioningConfig::Provision provision;
    provision.set(ZoneProvisioningConfig::File({VSMFILE_DIRECTORY,
                                    regularFile.parent_path().string(),
                                    0,
                                    0777}));
    config.provisions.push_back(provision);

    provision.set(ZoneProvisioningConfig::File({VSMFILE_REGULAR,
                                    regularFile.string(),
                                    O_CREAT,
                                    0777}));
    config.provisions.push_back(provision);

    provision.set(ZoneProvisioningConfig::File({VSMFILE_DIRECTORY,
                                    copyFile.parent_path().string(),
                                    0,
                                    0777}));
    config.provisions.push_back(provision);
    provision.set(ZoneProvisioningConfig::File({VSMFILE_REGULAR,
                                    copyFile.string(),
                                    0,
                                    0777}));
    config.provisions.push_back(provision);
    save(config);

    ZoneProvision zoneProvision = create({});
    zoneProvision.start();

    BOOST_CHECK(fs::exists(ROOTFS_PATH / regularFile.parent_path()));
    BOOST_CHECK(fs::exists(ROOTFS_PATH / regularFile));
    BOOST_CHECK(fs::exists(ROOTFS_PATH / copyFile.parent_path()));
    BOOST_CHECK(fs::exists(ROOTFS_PATH / copyFile));

    zoneProvision.stop();
}

BOOST_AUTO_TEST_CASE(Mount)
{
    //TODO: Test Fifo
    const fs::path mountTarget = fs::path("/opt/usr/data/ut-from-host-provision");
    const fs::path mountSource = fs::path("/tmp/ut-provision");
    const fs::path sharedFile = fs::path("ut-regular-file");

    utils::ScopedDir provisionfs(mountSource.string());


    ZoneProvisioningConfig config;
    ZoneProvisioningConfig::Provision provision;
    provision.set(ZoneProvisioningConfig::File({VSMFILE_DIRECTORY,
                                    mountTarget.string(),
                                    0,
                                    0777}));
    config.provisions.push_back(provision);
    provision.set(ZoneProvisioningConfig::Mount({mountSource.string(),
                                      mountTarget.string(),
                                      "",
                                      MS_BIND,
                                      ""}));
    config.provisions.push_back(provision);
    provision.set(ZoneProvisioningConfig::File({VSMFILE_REGULAR,
                                    (mountTarget / sharedFile).string(),
                                    O_CREAT,
                                    0777}));
    config.provisions.push_back(provision);
    save(config);

    ZoneProvision zoneProvision = create({});
    zoneProvision.start();

    BOOST_CHECK(fs::exists(ROOTFS_PATH / mountTarget));
    BOOST_CHECK(fs::exists(ROOTFS_PATH / mountTarget / sharedFile));
    BOOST_CHECK(fs::exists(mountSource / sharedFile));

    zoneProvision.stop();
}

BOOST_AUTO_TEST_CASE(Link)
{
    const fs::path linkFile = fs::path("/ut-from-host-file.txt");

    ZoneProvisioningConfig config;
    ZoneProvisioningConfig::Provision provision;

    provision.set(ZoneProvisioningConfig::Link({SOME_FILE_PATH.string(),
                                     linkFile.string()}));
    config.provisions.push_back(provision);
    save(config);
    {
        ZoneProvision zoneProvision = create({});
        zoneProvision.start();

        BOOST_CHECK(!fs::exists(ROOTFS_PATH / linkFile));

        zoneProvision.stop();
    }
    {
        ZoneProvision zoneProvision = create({"/tmp/"});
        zoneProvision.start();

        BOOST_CHECK(fs::exists(ROOTFS_PATH / linkFile));

        zoneProvision.stop();
    }
}

BOOST_AUTO_TEST_CASE(DeclareFile)
{
    ZoneProvision zoneProvision = create({});
    zoneProvision.declareFile(1, "path", 0747, 0777);
    zoneProvision.declareFile(2, "path", 0747, 0777);

    ZoneProvisioningConfig config;
    load(config);
    BOOST_REQUIRE_EQUAL(config.provisions.size(), 2);
    BOOST_REQUIRE(config.provisions[0].is<ZoneProvisioningConfig::File>());
    BOOST_REQUIRE(config.provisions[1].is<ZoneProvisioningConfig::File>());
    const ZoneProvisioningConfig::File& provision = config.provisions[0].as<ZoneProvisioningConfig::File>();
    BOOST_CHECK_EQUAL(provision.type, 1);
    BOOST_CHECK_EQUAL(provision.path, "path");
    BOOST_CHECK_EQUAL(provision.flags, 0747);
    BOOST_CHECK_EQUAL(provision.mode, 0777);
}

BOOST_AUTO_TEST_CASE(DeclareMount)
{
    ZoneProvision zoneProvision = create({});
    zoneProvision.declareMount("/fake/path1", "/fake/path2", "tmpfs", 077, "fake");
    zoneProvision.declareMount("/fake/path2", "/fake/path2", "tmpfs", 077, "fake");
    BOOST_CHECK_EXCEPTION(zoneProvision.declareMount("/fake/path2", "/fake/path2", "tmpfs", 077, "fake"),
                          UtilsException,
                          WhatEquals("Provision already exists"));

    ZoneProvisioningConfig config;
    load(config);
    BOOST_REQUIRE_EQUAL(config.provisions.size(), 2);
    BOOST_REQUIRE(config.provisions[0].is<ZoneProvisioningConfig::Mount>());
    BOOST_REQUIRE(config.provisions[1].is<ZoneProvisioningConfig::Mount>());
    const ZoneProvisioningConfig::Mount& provision = config.provisions[0].as<ZoneProvisioningConfig::Mount>();
    BOOST_CHECK_EQUAL(provision.source, "/fake/path1");
    BOOST_CHECK_EQUAL(provision.target, "/fake/path2");
    BOOST_CHECK_EQUAL(provision.type, "tmpfs");
    BOOST_CHECK_EQUAL(provision.flags, 077);
    BOOST_CHECK_EQUAL(provision.data, "fake");
}

BOOST_AUTO_TEST_CASE(DeclareLink)
{
    ZoneProvision zoneProvision = create({});
    zoneProvision.declareLink("/fake/path1", "/fake/path2");
    zoneProvision.declareLink("/fake/path2", "/fake/path2");

    ZoneProvisioningConfig config;
    load(config);
    BOOST_REQUIRE_EQUAL(config.provisions.size(), 2);
    BOOST_REQUIRE(config.provisions[0].is<ZoneProvisioningConfig::Link>());
    BOOST_REQUIRE(config.provisions[1].is<ZoneProvisioningConfig::Link>());
    const ZoneProvisioningConfig::Link& provision = config.provisions[0].as<ZoneProvisioningConfig::Link>();
    BOOST_CHECK_EQUAL(provision.source, "/fake/path1");
    BOOST_CHECK_EQUAL(provision.target, "/fake/path2");
}

BOOST_AUTO_TEST_CASE(ProvisionedAlready)
{
    const fs::path dir = fs::path("/opt/usr/data/ut-from-host");
    const fs::path linkFile = fs::path("/ut-from-host-file.txt");
    const fs::path regularFile = fs::path("/opt/usr/data/ut-regular-file");

    ZoneProvisioningConfig config;
    ZoneProvisioningConfig::Provision provision;
    provision.set(ZoneProvisioningConfig::File({VSMFILE_DIRECTORY,
                                    dir.string(),
                                    0,
                                    0777}));
    config.provisions.push_back(provision);
    provision.set(ZoneProvisioningConfig::Link({SOME_FILE_PATH.string(),
                                     linkFile.string()}));
    config.provisions.push_back(provision);
    provision.set(ZoneProvisioningConfig::File({VSMFILE_REGULAR,
                                    regularFile.string(),
                                    O_CREAT,
                                    0777}));
    config.provisions.push_back(provision);
    save(config);

    ZoneProvision zoneProvision = create({"/tmp/"});
    zoneProvision.start();

    BOOST_CHECK(fs::exists(ROOTFS_PATH / dir));
    BOOST_CHECK(fs::exists(ROOTFS_PATH / regularFile));
    BOOST_CHECK(fs::is_empty(ROOTFS_PATH / regularFile));
    BOOST_CHECK(fs::exists(ROOTFS_PATH / linkFile));

    std::fstream file((ROOTFS_PATH / regularFile).string(), std::fstream::out);
    BOOST_REQUIRE(file.is_open());
    file << "touch" << std::endl;
    file.close();
    BOOST_REQUIRE(!fs::is_empty(ROOTFS_PATH / regularFile));

    zoneProvision.stop();

    BOOST_CHECK(fs::exists(ROOTFS_PATH / dir));
    BOOST_CHECK(fs::exists(ROOTFS_PATH / regularFile));
    BOOST_CHECK(!fs::is_empty(ROOTFS_PATH / regularFile));
    BOOST_CHECK(fs::exists(ROOTFS_PATH / linkFile));

    zoneProvision.start();

    BOOST_CHECK(!fs::is_empty(ROOTFS_PATH / regularFile));

    zoneProvision.stop();
}

BOOST_AUTO_TEST_CASE(List)
{
    std::vector<std::string> expected;
    ZoneProvision zoneProvision = create({});

    zoneProvision.declareFile(1, "path", 0747, 0777);
    zoneProvision.declareFile(2, "path", 0747, 0777);
    expected.push_back("file path 1 " + std::to_string(0747) + " " + std::to_string(0777));
    expected.push_back("file path 2 " + std::to_string(0747) + " " + std::to_string(0777));

    zoneProvision.declareMount("/fake/path1", "/fake/path2", "tmpfs", 077, "fake1");
    zoneProvision.declareMount("/fake/path1", "/fake/path2", "tmpfs", 077, "fake2");
    expected.push_back("mount /fake/path1 /fake/path2 tmpfs " + std::to_string(077) + " fake1");
    expected.push_back("mount /fake/path1 /fake/path2 tmpfs " + std::to_string(077) + " fake2");

    zoneProvision.declareLink("/fake/path1", "/fake/path3");
    zoneProvision.declareLink("/fake/path2", "/fake/path4");
    expected.push_back("link /fake/path1 /fake/path3");
    expected.push_back("link /fake/path2 /fake/path4");

    const std::vector<std::string> provisions = zoneProvision.list();
    BOOST_REQUIRE_EQUAL(provisions.size(), expected.size());
    auto provision = provisions.cbegin();
    for (const auto& item : expected) {
        BOOST_CHECK_EQUAL(item, *(provision++));
    }
}

BOOST_AUTO_TEST_CASE(Remove)
{
    std::vector<std::string> expected;
    ZoneProvision zoneProvision = create({});

    zoneProvision.declareFile(1, "path", 0747, 0777);
    zoneProvision.declareFile(2, "path", 0747, 0777);
    expected.push_back("file path 2 " + std::to_string(0747) + " " + std::to_string(0777));

    zoneProvision.declareMount("/fake/path1", "/fake/path2", "tmpfs", 077, "fake1");
    zoneProvision.declareMount("/fake/path1", "/fake/path2", "tmpfs", 077, "fake2");
    expected.push_back("mount /fake/path1 /fake/path2 tmpfs " + std::to_string(077) + " fake1");

    zoneProvision.declareLink("/fake/path1", "/fake/path3");
    zoneProvision.declareLink("/fake/path2", "/fake/path4");
    expected.push_back("link /fake/path1 /fake/path3");

    zoneProvision.remove("file path 1 " + std::to_string(0747) + " " + std::to_string(0777));
    zoneProvision.remove("mount /fake/path1 /fake/path2 tmpfs " + std::to_string(077) + " fake2");
    zoneProvision.remove("link /fake/path2 /fake/path4");
    BOOST_CHECK_EXCEPTION(zoneProvision.remove("link /fake/path_fake /fake/path2"),
                          UtilsException,
                          WhatEquals("Can't find provision"));

    const std::vector<std::string> provisions = zoneProvision.list();
    BOOST_REQUIRE_EQUAL(provisions.size(), expected.size());
    auto provision = provisions.cbegin();
    for (const auto& item : expected) {
        BOOST_CHECK_EQUAL(item, *(provision++));
    }
}

BOOST_AUTO_TEST_SUITE_END()
