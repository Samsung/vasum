/*
 *  Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Krzysztof Dynowski (k.dynowski@samsumg.com)
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
 * @author  Krzysztof Dynowski (k.dynowski@samsung.com)
 * @brief   Unit tests of lxcpp cgroups managment
 */

#include "config.hpp"
#include "cargo-json/cargo-json.hpp"
#include "ut.hpp"
#include "logger/logger.hpp"

#include "lxcpp/exception.hpp"
#include "lxcpp/process.hpp"
#include "lxcpp/cgroups/devices.hpp"
#include "lxcpp/cgroups/cgroup-config.hpp"

#include "lxcpp/commands/cgroups.hpp"

#include "utils/text.hpp"

namespace {

struct Fixture {
    Fixture() {}
    ~Fixture() {}
};

} // namespace

BOOST_FIXTURE_TEST_SUITE(LxcppCGroupsSuite, Fixture)

using namespace lxcpp;

// assume cgroups are supprted by the system
BOOST_AUTO_TEST_CASE(GetAvailable)
{
    std::vector<std::string> subs;

    BOOST_CHECK_NO_THROW(subs = Subsystem::availableSubsystems());
    BOOST_CHECK_MESSAGE(subs.size() > 0, "Control groups not supported");
    for (auto n : subs){
        Subsystem s(n);
        LOGD(s.getName() << ": " << (s.isAttached() ? s.getMountPoint() : "[not attached]"));
    }
}

BOOST_AUTO_TEST_CASE(GetCGroupsByPid)
{
    std::vector<std::string> cg;

    BOOST_CHECK_NO_THROW(cg=Subsystem::getCGroups(::getpid()));
    BOOST_CHECK(cg.size() > 0);
}

BOOST_AUTO_TEST_CASE(GetPidsByCGroup)
{
    CGroup cg = CGroup::getCGroup("memory", ::getpid());
    std::vector<pid_t> pids;

    BOOST_CHECK_NO_THROW(pids = cg.getPids());
    BOOST_CHECK(pids.size() > 0);
}

BOOST_AUTO_TEST_CASE(SubsysAttach)
{
    Subsystem sub("freezer");

    BOOST_CHECK_MESSAGE(sub.isAvailable(), "freezer not found");

    if (!sub.isAvailable()) return ;

    if (sub.isAttached()) {
        std::string mp = sub.getMountPoint();
        BOOST_CHECK_NO_THROW(Subsystem::detach(mp));
        BOOST_CHECK(Subsystem(sub.getName()).isAttached()==false);
        BOOST_CHECK_NO_THROW(Subsystem::attach(mp, {sub.getName()}));
        BOOST_CHECK(Subsystem(sub.getName()).isAttached()==true);
    }
    else {
        std::string mp = "/sys/fs/cgroup/" + sub.getName();
        BOOST_CHECK_NO_THROW(Subsystem::attach(mp, {sub.getName()}));
        BOOST_CHECK(Subsystem(sub.getName()).isAttached()==true);
        BOOST_CHECK_NO_THROW(Subsystem::detach(mp));
        BOOST_CHECK(Subsystem(sub.getName()).isAttached()==false);
    }
}

#define CHILD_CHECK_NO_THROW(passed,code) try{code;}catch(...){passed=0;}
#define CHILD_CHECK_THROW(passed,code,exception) try{code;passed=0;}catch(const exception& _ex){}catch(...){passed=0;}
#define CHILD_CHECK(passed,cond) try{if (cond){}else passed=0;}catch(...){passed=0;}

BOOST_AUTO_TEST_CASE(ModifyCGroupParams)
{
    CGroup memg("memory:/ut-params");
    if (memg.exists()) {
        memg.destroy();
    }

    // this test can fail if prev. test round left unexpected
    BOOST_CHECK_MESSAGE(memg.exists() == false, "Cgroup alredy exists");
    BOOST_CHECK_NO_THROW(memg.create());
    BOOST_CHECK(memg.exists() == true);

    if (!memg.exists()) {
        return ;
    }

    pid_t pid = lxcpp::fork();
    if (pid == 0) {
        try {
            int status = 1;
            CHILD_CHECK_NO_THROW(status, memg.assignPid(::getpid()));
            CHILD_CHECK_NO_THROW(status, memg.assignGroup(::getpid()));

            CHILD_CHECK_NO_THROW(status, memg.setValue("limit_in_bytes", "256k"));
            CHILD_CHECK_NO_THROW(status, memg.setValue("soft_limit_in_bytes", "32k"));
            CHILD_CHECK_THROW(status, memg.getValue("non-existing-name"), CGroupException);
            CHILD_CHECK_THROW(status, memg.setValue("non-existing-name", "xxx"), CGroupException);

            LOGD("limit_in_bytes = " + memg.getValue("limit_in_bytes"));
            LOGD("soft_limit_in_bytes = " + memg.getValue("soft_limit_in_bytes"));
            CHILD_CHECK(status, 256*1024 == std::stoul(memg.getValue("limit_in_bytes")));
            CHILD_CHECK(status, 32*1024 == std::stoul(memg.getValue("soft_limit_in_bytes")));

            CGroup memtop("memory:/");
            memtop.assignPid(::getpid());
            memtop.setCommonValue("procs", std::to_string(::getpid()));
            ::_exit(status ? EXIT_SUCCESS : EXIT_FAILURE);
        } catch(...) {
            ::_exit(EXIT_FAILURE);
        }
    }
    BOOST_CHECK(lxcpp::waitpid(pid) == EXIT_SUCCESS);
    BOOST_CHECK_NO_THROW(memg.destroy());
}

BOOST_AUTO_TEST_CASE(ListDevicesPermissions)
{
    DevicesCGroup devgrp("/tmp");

    BOOST_CHECK_NO_THROW(devgrp.create());

    std::vector<DevicePermission> list;
    BOOST_CHECK_NO_THROW(list = devgrp.list());
    for (__attribute__((unused)) const auto& i : list) {
        LOGD(std::string("perm = ") + i.type + " " +
             std::to_string(i.major) + ":" + std::to_string(i.minor) + " " + i.permission);
    }

    BOOST_CHECK_NO_THROW(devgrp.destroy());
}

BOOST_AUTO_TEST_CASE(CGroupConfigSerialization)
{
    CGroupsConfig cfg;
    std::string tmpConfigFile = "/tmp/cgconfig.conf";

    BOOST_CHECK_NO_THROW(cargo::saveToJsonString(cfg));

    cfg.subsystems.push_back(SubsystemConfig{"cpu", "/tmp/cgroup/cpu"});

    CGroupConfig cpucfg = {"cpu", "/testcpu", {}, {}};
    cfg.cgroups.push_back(cpucfg);
    cargo::saveToJsonFile(tmpConfigFile, cfg);

    CGroupsConfig cfg2;
    BOOST_CHECK_NO_THROW(cargo::loadFromJsonFile(tmpConfigFile, cfg2));
    BOOST_CHECK(cfg2.subsystems.size() == cfg.subsystems.size());
}

BOOST_AUTO_TEST_CASE(CGroupCommands)
{
    CGroupsConfig cfg;
    Subsystem sub("cpu");
    std::string mp = sub.isAttached() ? sub.getMountPoint() : "/tmp/cgroup/cpu";

    cfg.subsystems.push_back(SubsystemConfig{"cpu", mp});
    CGroupConfig cpucfg = {"cpu", "/testcpu", {}, {}};
    cfg.cgroups.push_back(cpucfg);

    CGroupMakeAll cmd(cfg);
    BOOST_CHECK_NO_THROW(cmd.execute());

    CGroup cpugrp("cpu", "/testcpu");
    BOOST_CHECK_NO_THROW(cpugrp.destroy());
}

BOOST_AUTO_TEST_SUITE_END()
