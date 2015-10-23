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
 * @author  Krzysztof Dynowski (k.dynowski@samsung.com)
 * @brief   Unit tests of lxcpp cgroups managment
 */

#include "config.hpp"
#include "config/manager.hpp"
#include "ut.hpp"
#include "logger/logger.hpp"

#include "lxcpp/cgroups/devices.hpp"
#include "lxcpp/cgroups/cgroup-config.hpp"

#include "lxcpp/commands/cgroups.hpp"

#include "lxcpp/exception.hpp"
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

BOOST_AUTO_TEST_CASE(ControlGroupParams)
{
    CGroup memg("memory:/ut-params");

    // this test can fail if prev. test round left unexpected
    BOOST_CHECK_MESSAGE(memg.exists() == false, "Cgroup alredy exists");
    BOOST_CHECK_NO_THROW(memg.create());
    BOOST_CHECK(memg.exists() == true);

    if (!memg.exists()) {
        return ;
    }

    BOOST_CHECK_NO_THROW(memg.assignPid(::getpid()));
    BOOST_CHECK_NO_THROW(memg.assignGroup(::getpid()));

    BOOST_CHECK_NO_THROW(memg.setValue("limit_in_bytes", "10k"));
    BOOST_CHECK_NO_THROW(memg.setValue("soft_limit_in_bytes", "10k"));
    BOOST_CHECK_THROW(memg.getValue("non-existing-name"), CGroupException);
    BOOST_CHECK_THROW(memg.setValue("non-existing-name", "xxx"), CGroupException);

    LOGD("limit_in_bytes: " << memg.getValue("limit_in_bytes"));
    LOGD("soft_limit_in_bytes: " << memg.getValue("soft_limit_in_bytes"));
    LOGD("max_usage_in_bytes: " << memg.getValue("max_usage_in_bytes"));

    CGroup memtop("memory:/");
    memtop.assignPid(::getpid());
    memtop.setCommonValue("procs", std::to_string(::getpid()));

    BOOST_CHECK_NO_THROW(memg.destroy());
}

BOOST_AUTO_TEST_CASE(DevicesParams)
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

    BOOST_CHECK_NO_THROW(config::saveToJsonString(cfg));

    cfg.subsystems.push_back(SubsystemConfig{"cpu", "/tmp/cgroup/cpu"});

    CGroupConfig cpucfg = {"cpu", "/testcpu", {}, {}};
    cfg.cgroups.push_back(cpucfg);
    config::saveToJsonFile(tmpConfigFile, cfg);

    CGroupsConfig cfg2;
    BOOST_CHECK_NO_THROW(config::loadFromJsonFile(tmpConfigFile, cfg2));
    BOOST_CHECK(cfg2.subsystems.size()==cfg.subsystems.size());
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
