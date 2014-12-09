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
 * @brief   Unit tests of the ZoneProvsion class
 */

#include "config.hpp"
#include "ut.hpp"

#include "zone.hpp"

#include "utils/glib-loop.hpp"
#include "utils/scoped-dir.hpp"
#include "config/manager.hpp"
#include "zone-provision-config.hpp"
#include "vasum-client.h"

#include <memory>
#include <string>

#include <boost/filesystem.hpp>
#include <sys/mount.h>
#include <fcntl.h>

using namespace vasum;
using namespace config;

namespace fs = boost::filesystem;

namespace {

const std::string PROVISON_CONFIG_FILE = "provision.conf";
const std::string ZONE = "ut-zone-test";
const fs::path TEST_CONFIG_PATH = VSM_TEST_CONFIG_INSTALL_DIR "/server/ut-zone/zones/test.conf";
const fs::path ZONES_PATH = "/tmp/ut-zones";
const fs::path LXC_TEMPLATES_PATH = VSM_TEST_LXC_TEMPLATES_INSTALL_DIR;
const fs::path ZONE_PATH = ZONES_PATH / fs::path(ZONE);
const fs::path PROVISION_FILE_PATH = ZONE_PATH / fs::path(PROVISON_CONFIG_FILE);
const fs::path ROOTFS_PATH = ZONE_PATH / fs::path("rootfs");

struct Fixture {
    utils::ScopedGlibLoop mLoop;
    utils::ScopedDir mZonesPathGuard;
    utils::ScopedDir mRunGuard;
    utils::ScopedDir mRootfsPath;

    Fixture()
        : mZonesPathGuard(ZONES_PATH.string())
        , mRunGuard("/tmp/ut-run")
        , mRootfsPath(ROOTFS_PATH.string())
    {
    }

    std::unique_ptr<Zone> create(const std::string& configPath)
    {
        return std::unique_ptr<Zone>(new Zone(utils::Worker::create(),
                                              ZONES_PATH.string(),
                                              configPath,
                                              LXC_TEMPLATES_PATH.string(),
                                              ""));
    }
};

} // namespace


BOOST_FIXTURE_TEST_SUITE(ZoneProvisionSuite, Fixture)

BOOST_AUTO_TEST_CASE(FileTest)
{
    //TODO: Test Fifo
    const fs::path regularFile = fs::path("/opt/usr/data/ut-regular-file");
    const fs::path copyFile = PROVISION_FILE_PATH;

    ZoneProvisioning config;
    ZoneProvisioning::Unit unit;
    unit.set(ZoneProvisioning::File({VSMFILE_DIRECTORY,
                                    regularFile.parent_path().string(),
                                    0,
                                    0777}));
    config.units.push_back(unit);

    unit.set(ZoneProvisioning::File({VSMFILE_REGULAR,
                                    regularFile.string(),
                                    O_CREAT,
                                    0777}));
    config.units.push_back(unit);

    unit.set(ZoneProvisioning::File({VSMFILE_DIRECTORY,
                                    copyFile.parent_path().string(),
                                    0,
                                    0777}));
    config.units.push_back(unit);
    unit.set(ZoneProvisioning::File({VSMFILE_REGULAR,
                                    copyFile.string(),
                                    0,
                                    0777}));
    config.units.push_back(unit);
    config::saveToFile(PROVISION_FILE_PATH.string(), config);
    auto c = create(TEST_CONFIG_PATH.string());
    c->start();

    BOOST_CHECK(fs::exists(ROOTFS_PATH / regularFile.parent_path()));
    BOOST_CHECK(fs::exists(ROOTFS_PATH / regularFile));
    BOOST_CHECK(fs::exists(ROOTFS_PATH / copyFile.parent_path()));
    BOOST_CHECK(fs::exists(ROOTFS_PATH / copyFile));

    c->stop();
}

BOOST_AUTO_TEST_CASE(MountTest)
{
    //TODO: Test Fifo
    const fs::path mountTarget = fs::path("/opt/usr/data/ut-from-host-provision");
    const fs::path mountSource = fs::path("/tmp/ut-provision");
    const fs::path sharedFile = fs::path("ut-regular-file");

    utils::ScopedDir provisionfs(mountSource.string());


    ZoneProvisioning config;
    ZoneProvisioning::Unit unit;
    unit.set(ZoneProvisioning::File({VSMFILE_DIRECTORY,
                                    mountTarget.string(),
                                    0,
                                    0777}));
    config.units.push_back(unit);
    unit.set(ZoneProvisioning::Mount({mountSource.string(),
                                      mountTarget.string(),
                                      "",
                                      MS_BIND,
                                      ""}));
    config.units.push_back(unit);
    unit.set(ZoneProvisioning::File({VSMFILE_REGULAR,
                                    (mountTarget / sharedFile).string(),
                                    O_CREAT,
                                    0777}));
    config.units.push_back(unit);

    config::saveToFile(PROVISION_FILE_PATH.string(), config);
    auto c = create(TEST_CONFIG_PATH.string());
    c->start();

    BOOST_CHECK(fs::exists(ROOTFS_PATH / mountTarget));
    BOOST_CHECK(fs::exists(ROOTFS_PATH / mountTarget / sharedFile));
    BOOST_CHECK(fs::exists(mountSource / sharedFile));

    c->stop();
}

BOOST_AUTO_TEST_CASE(LinkTest)
{
    const fs::path linkFile = fs::path("/ut-from-host-provision.conf");

    ZoneProvisioning config;
    ZoneProvisioning::Unit unit;

    unit.set(ZoneProvisioning::Link({PROVISION_FILE_PATH.string(),
                                     linkFile.string()}));
    config.units.push_back(unit);
    config::saveToFile(PROVISION_FILE_PATH.string(), config);
    auto c = create(TEST_CONFIG_PATH.string());
    c->start();

    BOOST_CHECK(fs::exists(ROOTFS_PATH / linkFile));

    c->stop();
}

BOOST_AUTO_TEST_CASE(DeclareFile)
{
    ZoneProvision zoneProvision(ZONE_PATH.string(), {});
    zoneProvision.declareFile(1, "path", 0747, 0777);
    zoneProvision.declareFile(2, "path", 0747, 0777);

    ZoneProvisioning config;
    BOOST_REQUIRE_NO_THROW(loadFromFile(PROVISION_FILE_PATH.string(), config));
    BOOST_REQUIRE_EQUAL(config.units.size(), 2);
    BOOST_REQUIRE(config.units[0].is<ZoneProvisioning::File>());
    BOOST_REQUIRE(config.units[1].is<ZoneProvisioning::File>());
    const ZoneProvisioning::File& unit = config.units[0].as<ZoneProvisioning::File>();
    BOOST_CHECK_EQUAL(unit.type, 1);
    BOOST_CHECK_EQUAL(unit.path, "path");
    BOOST_CHECK_EQUAL(unit.flags, 0747);
    BOOST_CHECK_EQUAL(unit.mode, 0777);
}

BOOST_AUTO_TEST_CASE(DeclareMount)
{
    ZoneProvision zoneProvision(ZONE_PATH.string(), {});
    zoneProvision.declareMount("/fake/path1", "/fake/path2", "tmpfs", 077, "fake");
    zoneProvision.declareMount("/fake/path2", "/fake/path2", "tmpfs", 077, "fake");

    ZoneProvisioning config;
    BOOST_REQUIRE_NO_THROW(loadFromFile(PROVISION_FILE_PATH.string(), config));
    BOOST_REQUIRE_EQUAL(config.units.size(), 2);
    BOOST_REQUIRE(config.units[0].is<ZoneProvisioning::Mount>());
    BOOST_REQUIRE(config.units[1].is<ZoneProvisioning::Mount>());
    const ZoneProvisioning::Mount& unit = config.units[0].as<ZoneProvisioning::Mount>();
    BOOST_CHECK_EQUAL(unit.source, "/fake/path1");
    BOOST_CHECK_EQUAL(unit.target, "/fake/path2");
    BOOST_CHECK_EQUAL(unit.type, "tmpfs");
    BOOST_CHECK_EQUAL(unit.flags, 077);
    BOOST_CHECK_EQUAL(unit.data, "fake");
}

BOOST_AUTO_TEST_CASE(DeclareLink)
{
    ZoneProvision zoneProvision(ZONE_PATH.string(), {});
    zoneProvision.declareLink("/fake/path1", "/fake/path2");
    zoneProvision.declareLink("/fake/path2", "/fake/path2");

    ZoneProvisioning config;
    BOOST_REQUIRE_NO_THROW(loadFromFile(PROVISION_FILE_PATH.string(), config));
    BOOST_REQUIRE_EQUAL(config.units.size(), 2);
    BOOST_REQUIRE(config.units[0].is<ZoneProvisioning::Link>());
    BOOST_REQUIRE(config.units[1].is<ZoneProvisioning::Link>());
    const ZoneProvisioning::Link& unit = config.units[0].as<ZoneProvisioning::Link>();
    BOOST_CHECK_EQUAL(unit.source, "/fake/path1");
    BOOST_CHECK_EQUAL(unit.target, "/fake/path2");
}

BOOST_AUTO_TEST_SUITE_END()
