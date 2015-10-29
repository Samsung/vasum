/*
 *  Copyright (C) 2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License version 2.1 as published by the Free Software Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

/**
 * @file
 * @author  Maciej Karpiuk (m.karpiuk2@samsung.com)
 * @brief   Unit tests of lxcpp provisioning
 */

#include "config.hpp"
#include "config/manager.hpp"
#include "lxcpp/lxcpp.hpp"
#include "lxcpp/container.hpp"
#include "lxcpp/container-config.hpp"
#include "lxcpp/provision-config.hpp"
#include "ut.hpp"
#include "utils/scoped-dir.hpp"
#include <iostream>

namespace {

using namespace lxcpp;
using namespace config;
using namespace provision;

const std::string TEST_DIR = "/tmp/ut-provisioning";
const std::string ROOT_DIR = TEST_DIR + "/root";
const std::string WORK_DIR = TEST_DIR + "/work";

struct Fixture {

    Fixture()
        : mTestPath(ROOT_DIR),
          mRoot(ROOT_DIR),
          mWork(WORK_DIR)
    {
        container = std::unique_ptr<Container>(createContainer("ProvisioningTester", ROOT_DIR, WORK_DIR));
    }
    ~Fixture() {}

    std::unique_ptr<Container> container;

    utils::ScopedDir mTestPath;
    utils::ScopedDir mRoot;
    utils::ScopedDir mWork;
};

} // namespace


BOOST_FIXTURE_TEST_SUITE(LxcppProvisioningSuite, Fixture)

BOOST_AUTO_TEST_CASE(ListProvisionsEmptyContainer)
{
    BOOST_REQUIRE(container->getFiles().size() == 0);
    BOOST_REQUIRE(container->getMounts().size() == 0);
    BOOST_REQUIRE(container->getLinks().size() == 0);
}

BOOST_AUTO_TEST_CASE(AddDeclareFile)
{
    container->declareFile(File::Type::FIFO, "path", 0747, 0777);
    container->declareFile(File::Type::REGULAR, "path", 0747, 0777);

    std::vector<File> fileList = container->getFiles();
    BOOST_REQUIRE_EQUAL(fileList.size(), 2);

    BOOST_REQUIRE(fileList[0].type == File::Type::FIFO);
    BOOST_REQUIRE(fileList[0].path == "path");
    BOOST_REQUIRE(fileList[0].flags == 0747);
    BOOST_REQUIRE(fileList[0].mode == 0777);
    BOOST_REQUIRE(fileList[1].type == File::Type::REGULAR);

    BOOST_REQUIRE_NO_THROW(container->removeFile(fileList[0]));
    BOOST_REQUIRE_EQUAL(container->getFiles().size(), 1);
    File dummyFile({File::Type::FIFO, "dummy", 1, 2});
    BOOST_REQUIRE_THROW(container->removeFile(dummyFile), ProvisionException);
    BOOST_REQUIRE_NO_THROW(container->removeFile(fileList[1]));
    BOOST_REQUIRE_EQUAL(container->getFiles().size(), 0);
}

BOOST_AUTO_TEST_CASE(AddDeclareMount)
{
    container->declareMount("/fake/path1", "/fake/path2", "tmpfs", 077, "fake");
    container->declareMount("/fake/path2", "/fake/path2", "tmpfs", 077, "fake");
    BOOST_CHECK_THROW(container->declareMount("/fake/path2", "/fake/path2", "tmpfs", 077, "fake"),
                      ProvisionException);

    std::vector<Mount> mountList = container->getMounts();
    BOOST_REQUIRE_EQUAL(mountList.size(), 2);

    BOOST_REQUIRE(mountList[0].source == "/fake/path1");
    BOOST_REQUIRE(mountList[0].target == "/fake/path2");
    BOOST_REQUIRE(mountList[0].type == "tmpfs");
    BOOST_REQUIRE(mountList[0].flags == 077);
    BOOST_REQUIRE(mountList[0].data == "fake");

    BOOST_REQUIRE(mountList[1].source == "/fake/path2");
    BOOST_REQUIRE(mountList[1].target == "/fake/path2");
    BOOST_REQUIRE(mountList[1].type == "tmpfs");
    BOOST_REQUIRE(mountList[1].flags == 077);
    BOOST_REQUIRE(mountList[1].data == "fake");

    BOOST_REQUIRE_NO_THROW(container->removeMount(mountList[0]));
    BOOST_REQUIRE_EQUAL(container->getMounts().size(), 1);
    Mount dummyMount({"a", "b", "c", 1, "d"});
    BOOST_REQUIRE_THROW(container->removeMount(dummyMount), ProvisionException);
    BOOST_REQUIRE_NO_THROW(container->removeMount(mountList[1]));
    BOOST_REQUIRE_EQUAL(container->getMounts().size(), 0);
}

BOOST_AUTO_TEST_CASE(AddDeclareLink)
{
    container->declareLink("/fake/path1", "/fake/path2");
    container->declareLink("/fake/path2", "/fake/path2");
    BOOST_CHECK_THROW(container->declareLink("/fake/path2", "/fake/path2"),
                      ProvisionException);

    std::vector<Link> linkList = container->getLinks();
    BOOST_REQUIRE_EQUAL(linkList.size(), 2);

    BOOST_REQUIRE(linkList[0].source == "/fake/path1");
    BOOST_REQUIRE(linkList[0].target == "/fake/path2");
    BOOST_REQUIRE(linkList[1].source == "/fake/path2");
    BOOST_REQUIRE(linkList[1].target == "/fake/path2");

    BOOST_REQUIRE_NO_THROW(container->removeLink(linkList[0]));
    BOOST_REQUIRE_EQUAL(container->getLinks().size(), 1);
    Link dummyLink({"a", "b"});
    BOOST_REQUIRE_THROW(container->removeLink(dummyLink), ProvisionException);
    BOOST_REQUIRE_NO_THROW(container->removeLink(linkList[1]));
    BOOST_REQUIRE_EQUAL(container->getLinks().size(), 0);
}

BOOST_AUTO_TEST_CASE(ProvisioningConfigSerialization)
{
    std::string tmpConfigFile = "/tmp/fileconfig.conf";
    std::string tmpConfigMount = "/tmp/mountconfig.conf";
    std::string tmpConfigLink = "/tmp/linkconfig.conf";

    File  saved_file ({File::Type::REGULAR, "path", 0747, 0777});
    Mount saved_mount({"/fake/path1", "/fake/path2", "tmpfs", 077, "fake"});
    Link  saved_link ({"/fake/path1", "/fake/path2"});
    config::saveToJsonFile(tmpConfigFile, saved_file);
    config::saveToJsonFile(tmpConfigMount, saved_mount);
    config::saveToJsonFile(tmpConfigLink, saved_link);

    File loaded_file;
    Mount loaded_mount;
    Link loaded_link;
    config::loadFromJsonFile(tmpConfigFile, loaded_file);
    config::loadFromJsonFile(tmpConfigMount, loaded_mount);
    config::loadFromJsonFile(tmpConfigLink, loaded_link);
    BOOST_REQUIRE(saved_file == loaded_file);
    BOOST_REQUIRE(saved_mount == loaded_mount);
    BOOST_REQUIRE(saved_link == loaded_link);
}

BOOST_AUTO_TEST_SUITE_END()
