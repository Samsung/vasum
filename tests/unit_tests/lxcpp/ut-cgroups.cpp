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
#include "ut.hpp"
#include "logger/logger.hpp"

#include "lxcpp/cgroups/subsystem.hpp"

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
    BOOST_CHECK(subs.size() > 0);
    for (auto n : subs){
        Subsystem s(n);
        LOGD(s.getName() << ": " << (s.isAttached()?s.getMountPoint():"[not attached]"));
    }
}

BOOST_AUTO_TEST_CASE(GetCGroupsByPid)
{
    std::vector<std::string> cg;
    BOOST_CHECK_NO_THROW(cg=Subsystem::getCGroups(::getpid()));
    BOOST_CHECK(cg.size() > 0);
}

BOOST_AUTO_TEST_CASE(SubsysAttach)
{
    Subsystem sub("freezer");
    BOOST_CHECK_MESSAGE(sub.getName() == "freezer", "freezer not equal");
    BOOST_CHECK_MESSAGE(sub.isAvailable(), "freezer not found");

    if (!sub.isAvailable()) return ;


    if (sub.isAttached()) {
        std::string mp = sub.getMountPoint();
        BOOST_CHECK_NO_THROW(Subsystem::detach(mp));
        BOOST_CHECK_NO_THROW(Subsystem::attach(mp, {sub.getName()}));
    }
    else {
        std::string mp = "/sys/fs/cgroup/" + sub.getName();
        BOOST_CHECK_NO_THROW(Subsystem::attach(mp, {sub.getName()}));
        BOOST_CHECK_NO_THROW(Subsystem::detach(mp));
    }
}

BOOST_AUTO_TEST_SUITE_END()
