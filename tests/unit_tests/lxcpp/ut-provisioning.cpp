/*
 *  Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Maciej Karpiuk (m.karpiuk2@samsung.com)
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
 * @author  Maciej Karpiuk (m.karpiuk2@samsung.com)
 * @brief   Unit tests of lxcpp provisioning
 */

#include "config.hpp"

#include "ut.hpp"
#include "cargo-json/cargo-json.hpp"
#include "lxcpp/lxcpp.hpp"
#include "lxcpp/container.hpp"
#include "lxcpp/container-config.hpp"
#include "lxcpp/provision-config.hpp"

#include "utils/scoped-dir.hpp"
#include "utils/exception.hpp"
#include "utils/fs.hpp"
#include "utils/spin-wait-for.hpp"
#include <sys/mount.h>
#include <iostream>

namespace {

using namespace lxcpp;
using namespace cargo;
using namespace provision;

const std::string ROOT_DIR            = "/";
const std::string TEST_DIR            = "/tmp/ut-provisioning/";
const std::string WORK_DIR            = "/tmp/ut-work/";
const std::string EXTERNAL_DIR        = "/tmp/ut-temporary/";
const std::string USR_BIN_DIR         = "/usr/bin/";
const std::string TEST_MOUNT_VIRT_DIR = TEST_DIR + "bin";
const std::string LOGGER_FILE         = TEST_DIR + "provision.log";
const std::string TEST_FILE           = "test_file";
const std::string TEST_EXT_FILE       = "bash";

const std::string TESTS_CMD_ROOT      = VSM_TEST_CONFIG_INSTALL_DIR "/utils/";
const std::string TEST_CMD_LIST       = "list_files.sh";
const std::string TEST_CMD_LIST_RET   = "/tmp/list_files_ret.txt";

const std::vector<std::string> COMMAND = {"/bin/bash",
                                          "-c", "trap exit SIGTERM; while true; do sleep 0.1; done"
                                         };
// const int TIMEOUT = 3000; //ms

struct Fixture {
    Fixture() : mTestPath(TEST_DIR), mWork(WORK_DIR)
    {
        // setup test
        c = std::unique_ptr<Container>(createContainer("ProvisioningTester", ROOT_DIR, WORK_DIR));
        c->setLogger(logger::LogType::LOG_PERSISTENT_FILE,
                     logger::LogLevel::DEBUG,
                     LOGGER_FILE);
        c->setInit(COMMAND);

        // cleanup
        utils::removeFile(TEST_DIR + TEST_FILE);
    }
    ~Fixture() {}

    bool attachListFiles(const std::string& dir, const std::string& lookupItem) {
        bool found = false;

        c->attach({TESTS_CMD_ROOT + TEST_CMD_LIST, dir, TEST_CMD_LIST_RET},
                  0, 0,  // uid, gid
                  std::string(), // ttyPath
                  {},    // supplementaryGids
                  0,     // capsToKeep
                  TEST_DIR,
                  {},    // envToKeep
                  {}     // envToSet
                  );
        std::string file_list = utils::readFileContent(TEST_CMD_LIST_RET);
        if(file_list.find(lookupItem) != std::string::npos)
            found = true;
        utils::removeFile(TEST_CMD_LIST_RET);
        return found;
    }

    std::unique_ptr<Container> c;

    utils::ScopedDir mTestPath;
    utils::ScopedDir mWork;
};

struct MountFixture : Fixture {
    MountFixture() :  mExternalPath(EXTERNAL_DIR),
        mItem( {
        EXTERNAL_DIR, TEST_MOUNT_VIRT_DIR, "tmpfs", MS_BIND | MS_RDONLY, ""
    })
    {
        // cleanup
        utils::removeFile(TEST_DIR + TEST_EXT_FILE);

        // setup test
        utils::createDirs(mItem.target, 0777);
        utils::copyFile(USR_BIN_DIR + TEST_EXT_FILE, EXTERNAL_DIR + TEST_EXT_FILE);
    }
    ~MountFixture() {
        // race: who does the umount first? stopping container or test?
        // no matter who - we have to have it unmounted before ScopedDir is deleted.
        utils::umount(mItem.target);
    }

    void declareMount() {
        c->declareMount(mItem.source,
                        mItem.target,
                        mItem.type,
                        mItem.flags,
                        mItem.data);
    }
    utils::ScopedDir mExternalPath;
    Mount mItem;
};

} // namespace


BOOST_FIXTURE_TEST_SUITE(LxcppProvisioningSuite, Fixture)

// FIXME: Uncomment
# if 0

BOOST_AUTO_TEST_CASE(ListProvisionsEmptyContainer)
{
    BOOST_REQUIRE(c->getFiles().size() == 0);
    BOOST_REQUIRE(c->getMounts().size() == 0);
    BOOST_REQUIRE(c->getLinks().size() == 0);
}

BOOST_AUTO_TEST_CASE(AddDeclareFile)
{
    c->declareFile(File::Type::FIFO, "path", 0747, 0777);
    c->declareFile(File::Type::REGULAR, "path", 0747, 0777);

    std::vector<File> fileList = c->getFiles();
    BOOST_REQUIRE_EQUAL(fileList.size(), 2);

    BOOST_REQUIRE(fileList[0].type == File::Type::FIFO);
    BOOST_REQUIRE(fileList[0].path == "path");
    BOOST_REQUIRE(fileList[0].flags == 0747);
    BOOST_REQUIRE(fileList[0].mode == 0777);
    BOOST_REQUIRE(fileList[1].type == File::Type::REGULAR);

    BOOST_REQUIRE_NO_THROW(c->removeFile(fileList[0]));
    BOOST_REQUIRE_EQUAL(c->getFiles().size(), 1);
    File dummyFile({File::Type::FIFO, "dummy", 1, 2});
    BOOST_REQUIRE_THROW(c->removeFile(dummyFile), ProvisionException);
    BOOST_REQUIRE_NO_THROW(c->removeFile(fileList[1]));
    BOOST_REQUIRE_EQUAL(c->getFiles().size(), 0);
}

BOOST_AUTO_TEST_CASE(AddDeclareMount)
{
    c->declareMount("/fake/path1", "/fake/path2", "tmpfs", 077, "fake");
    c->declareMount("/fake/path2", "/fake/path2", "tmpfs", 077, "fake");
    BOOST_CHECK_THROW(c->declareMount("/fake/path2", "/fake/path2", "tmpfs", 077, "fake"),
                      ProvisionException);

    std::vector<Mount> mountList = c->getMounts();
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

    BOOST_REQUIRE_NO_THROW(c->removeMount(mountList[0]));
    BOOST_REQUIRE_EQUAL(c->getMounts().size(), 1);
    Mount dummyMount({"a", "b", "c", 1, "d"});
    BOOST_REQUIRE_THROW(c->removeMount(dummyMount), ProvisionException);
    BOOST_REQUIRE_NO_THROW(c->removeMount(mountList[1]));
    BOOST_REQUIRE_EQUAL(c->getMounts().size(), 0);
}

BOOST_AUTO_TEST_CASE(AddDeclareLink)
{
    c->declareLink("/fake/path1", "/fake/path2");
    c->declareLink("/fake/path2", "/fake/path2");
    BOOST_CHECK_THROW(c->declareLink("/fake/path2", "/fake/path2"),
                      ProvisionException);

    std::vector<Link> linkList = c->getLinks();
    BOOST_REQUIRE_EQUAL(linkList.size(), 2);

    BOOST_REQUIRE(linkList[0].source == "/fake/path1");
    BOOST_REQUIRE(linkList[0].target == "/fake/path2");
    BOOST_REQUIRE(linkList[1].source == "/fake/path2");
    BOOST_REQUIRE(linkList[1].target == "/fake/path2");

    BOOST_REQUIRE_NO_THROW(c->removeLink(linkList[0]));
    BOOST_REQUIRE_EQUAL(c->getLinks().size(), 1);
    Link dummyLink({"a", "b"});
    BOOST_REQUIRE_THROW(c->removeLink(dummyLink), ProvisionException);
    BOOST_REQUIRE_NO_THROW(c->removeLink(linkList[1]));
    BOOST_REQUIRE_EQUAL(c->getLinks().size(), 0);
}

BOOST_AUTO_TEST_CASE(ConfigSerialization)
{
    std::string tmpConfigFile = "/tmp/fileconfig.conf";
    std::string tmpConfigMount = "/tmp/mountconfig.conf";
    std::string tmpConfigLink = "/tmp/linkconfig.conf";

    File  saved_file ({File::Type::REGULAR, "path", 0747, 0777});
    Mount saved_mount({"/fake/path1", "/fake/path2", "tmpfs", 077, "fake"});
    Link  saved_link ({"/fake/path1", "/fake/path2"});
    cargo::saveToJsonFile(tmpConfigFile, saved_file);
    cargo::saveToJsonFile(tmpConfigMount, saved_mount);
    cargo::saveToJsonFile(tmpConfigLink, saved_link);

    File loaded_file;
    Mount loaded_mount;
    Link loaded_link;
    cargo::loadFromJsonFile(tmpConfigFile, loaded_file);
    cargo::loadFromJsonFile(tmpConfigMount, loaded_mount);
    cargo::loadFromJsonFile(tmpConfigLink, loaded_link);
    BOOST_REQUIRE(saved_file == loaded_file);
    BOOST_REQUIRE(saved_mount == loaded_mount);
    BOOST_REQUIRE(saved_link == loaded_link);
}

BOOST_AUTO_TEST_CASE(CreateFileOnStartup)
{
    BOOST_REQUIRE_THROW(utils::readFileContent(TEST_DIR + TEST_FILE), utils::UtilsException);

    c->declareFile(File::Type::REGULAR, TEST_DIR + TEST_FILE, 0747, 0777);
    std::vector<File> fileList = c->getFiles();
    BOOST_REQUIRE_EQUAL(fileList.size(), 1);

    BOOST_REQUIRE_NO_THROW(c->start());
    BOOST_REQUIRE(utils::spinWaitFor(TIMEOUT, [&] {return c->getState() == Container::State::RUNNING;}));
    BOOST_ASSERT(attachListFiles(TEST_DIR, TEST_FILE));
    BOOST_REQUIRE_NO_THROW(c->stop());
    BOOST_REQUIRE(utils::spinWaitFor(TIMEOUT, [&] {return c->getState() == Container::State::STOPPED;}));

    // file already exists, let's start again
    // to check what happens if file already exists
    auto helper = std::unique_ptr<Container>(createContainer("ProvisioningTesterHelper", ROOT_DIR, WORK_DIR));
    helper->setInit(COMMAND);
    BOOST_REQUIRE_NO_THROW(helper->start());
    BOOST_REQUIRE(utils::spinWaitFor(TIMEOUT, [&] {return helper->getState() == Container::State::RUNNING;}));
    BOOST_REQUIRE_NO_THROW(helper->stop());
    BOOST_REQUIRE(utils::spinWaitFor(TIMEOUT, [&] {return helper->getState() == Container::State::STOPPED;}));
    BOOST_REQUIRE_NO_THROW(utils::readFileContent(TEST_DIR + TEST_FILE));
}

BOOST_AUTO_TEST_CASE(CreateFileWhileRunning)
{
    BOOST_REQUIRE_THROW(utils::readFileContent(TEST_DIR + TEST_FILE), utils::UtilsException);
    BOOST_REQUIRE_EQUAL(c->getFiles().size(), 0);

    BOOST_REQUIRE_NO_THROW(c->start());
    BOOST_REQUIRE(utils::spinWaitFor(TIMEOUT, [&] {return c->getState() == Container::State::RUNNING;}));
    BOOST_ASSERT(attachListFiles(TEST_DIR, TEST_FILE) == false);
    c->declareFile(File::Type::REGULAR, TEST_DIR + TEST_FILE, 0747, 0777);
    BOOST_REQUIRE_EQUAL(c->getFiles().size(), 1);
    BOOST_ASSERT(attachListFiles(TEST_DIR, TEST_FILE));
    BOOST_REQUIRE_NO_THROW(c->stop());
    BOOST_REQUIRE(utils::spinWaitFor(TIMEOUT, [&] {return c->getState() == Container::State::STOPPED;}));
}

BOOST_FIXTURE_TEST_CASE(MountDirectory, MountFixture)
{
    BOOST_REQUIRE_NO_THROW(declareMount());
    BOOST_REQUIRE_NO_THROW(c->start());
    BOOST_REQUIRE(utils::spinWaitFor(TIMEOUT, [&] {return c->getState() == Container::State::RUNNING;}));
    BOOST_REQUIRE(attachListFiles(TEST_MOUNT_VIRT_DIR, TEST_EXT_FILE));
    BOOST_REQUIRE_NO_THROW(c->stop());
    BOOST_REQUIRE(utils::spinWaitFor(TIMEOUT, [&] {return c->getState() == Container::State::STOPPED;}));
}

BOOST_FIXTURE_TEST_CASE(MountUnmountDirectoryWhileRunning, MountFixture)
{
    std::vector<Mount> mountList = c->getMounts();
    BOOST_REQUIRE_EQUAL(mountList.size(), 0);

    BOOST_REQUIRE_NO_THROW(c->start());
    BOOST_REQUIRE(utils::spinWaitFor(TIMEOUT, [&] {return c->getState() == Container::State::RUNNING;}));

    // mount
    BOOST_REQUIRE_NO_THROW(declareMount());
    BOOST_REQUIRE(attachListFiles(TEST_MOUNT_VIRT_DIR, TEST_EXT_FILE));

    // unmount
    BOOST_REQUIRE_NO_THROW(c->removeMount(mItem));
    BOOST_REQUIRE(attachListFiles(TEST_MOUNT_VIRT_DIR, TEST_EXT_FILE) == false);

    BOOST_REQUIRE_NO_THROW(c->stop());
    BOOST_REQUIRE(utils::spinWaitFor(TIMEOUT, [&] {return c->getState() == Container::State::STOPPED;}));
}

BOOST_FIXTURE_TEST_CASE(LinkFile, Fixture)
{
    c->declareFile(File::Type::REGULAR, TEST_DIR + TEST_FILE, 0747, 0777);
    BOOST_REQUIRE_NO_THROW(c->declareLink(TEST_DIR + TEST_FILE,
                                          TEST_DIR + TEST_EXT_FILE));
    BOOST_REQUIRE_NO_THROW(c->start());
    BOOST_REQUIRE(utils::spinWaitFor(TIMEOUT, [&] {return c->getState() == Container::State::RUNNING;}));
    BOOST_ASSERT(attachListFiles(TEST_DIR, TEST_EXT_FILE));
    BOOST_REQUIRE_NO_THROW(c->stop());
    BOOST_REQUIRE(utils::spinWaitFor(TIMEOUT, [&] {return c->getState() == Container::State::STOPPED;}));
}

BOOST_AUTO_TEST_CASE(LinkFileWhileRunning)
{
    std::vector<Link> linkList = c->getLinks();
    BOOST_REQUIRE_EQUAL(linkList.size(), 0);

    BOOST_REQUIRE_NO_THROW(c->start());
    BOOST_REQUIRE(utils::spinWaitFor(TIMEOUT, [&] {return c->getState() == Container::State::RUNNING;}));
    c->declareFile(File::Type::REGULAR, TEST_DIR + TEST_FILE, 0747, 0777);
    BOOST_REQUIRE_NO_THROW(c->declareLink(TEST_DIR + TEST_FILE,
                                          TEST_DIR + TEST_EXT_FILE));
    BOOST_ASSERT(attachListFiles(TEST_DIR, TEST_EXT_FILE));
    BOOST_REQUIRE_NO_THROW(c->stop());
    BOOST_REQUIRE(utils::spinWaitFor(TIMEOUT, [&] {return c->getState() == Container::State::STOPPED;}));
}

# endif


BOOST_AUTO_TEST_SUITE_END()
